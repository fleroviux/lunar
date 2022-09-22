/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>

#include "gpu.hpp"

namespace lunar::nds {

void GPU::SubmitVertex(Vector4<Fixed20x12> const& position) {
  if (!in_vertex_list) {
    LOG_ERROR("GPU: cannot submit vertex data outside of VTX_BEGIN / VTX_END");
    return;
  }

  if (texture_params.transform == TextureParams::Transform::Position) {
    auto const& matrix = texture.current;

    auto x = position.x();
    auto y = position.y();
    auto z = position.z();

    auto t_x = Fixed20x12{vertex_uv_source.x().raw() << 12};
    auto t_y = Fixed20x12{vertex_uv_source.y().raw() << 12};

    vertex_uv = Vector2<Fixed12x4>{
      s16((x * matrix[0].x() + y * matrix[1].x() + z * matrix[2].x() + t_x).raw() >> 12),
      s16((x * matrix[0].y() + y * matrix[1].y() + z * matrix[2].y() + t_y).raw() >> 12)
    };
  }

  auto clip_position = clip_matrix * position;

  current_vertex_list.push_back({clip_position, vertex_color, vertex_uv});

  position_old = position;

  int required = is_quad ? 4 : 3;
  if (is_strip && !is_first) {
    required -= 2;
  }

  if (current_vertex_list.size() == required) {
    auto& poly_ram = polygons[buffer];
    auto& vert_ram = vertices[buffer];

    int polygon_count = poly_ram.count;

    if (polygon_count == 2048) {
      return;
    }

    auto& poly = poly_ram.data[polygon_count];

    bool needs_clipping = false;

    // Determine if the polygon needs to be clipped.
    for (auto const& v : current_vertex_list) {
      auto w = v.position[3];

      for (int i = 0; i < 3; i++) {
        if (v.position[i] < -w || v.position[i] > w) {
          needs_clipping = true;
          break;
        }
      }
    }

    // In a triangle strip the winding order of each second polygon is inverted.
    bool invert_winding = is_strip && !is_quad && (polygon_strip_length % 2) == 1;

    auto next_vertex_list = StaticVec<Vertex, 10>{};

    poly.count = 0;

    // Append the last two vertices from vertex RAM to the polygon.
    if (is_strip && !is_first) {
      if (needs_clipping) {
        // TODO: can we do this more efficiently?
        current_vertex_list.insert(current_vertex_list.begin(), vert_ram.data[vert_ram.count - 1]);
        current_vertex_list.insert(current_vertex_list.begin(), vert_ram.data[vert_ram.count - 2]);
      } else {
        poly.vertices[0] = &vert_ram.data[vert_ram.count - 2];
        poly.vertices[1] = &vert_ram.data[vert_ram.count - 1];
        poly.count = 2;
      }
    }

    bool front_facing;

    if (poly.count == 2) {
      auto const& v0 = poly.vertices[0]->position;
      auto const& v1 = current_vertex_list.back().position;
      auto const& v2 = poly.vertices[1]->position;

      front_facing = IsFrontFacing(v0, v1, v2, invert_winding);
    } else {
      auto const& v0 = current_vertex_list[0].position;
      auto const& v1 = current_vertex_list.back().position;
      auto const& v2 = current_vertex_list[1].position;

      front_facing = IsFrontFacing(v0, v1, v2, invert_winding);
    }

    bool cull = !((poly_params.render_front_side &&  front_facing) ||
                  (poly_params.render_back_side  && !front_facing));

    if (cull) {
      if (is_strip) {
        polygon_strip_length++;
      }

      if (needs_clipping && is_strip) {
        current_vertex_list.erase(current_vertex_list.begin());
        if (is_quad) {
          current_vertex_list.erase(current_vertex_list.begin());
        }

        is_first = true;
      } else {
        if (is_strip) {
          int size = (int)current_vertex_list.size();

          for (int i = std::max(0, size - 2); i < size; i++) {
            // TODO: can we do this more efficiently?
            if (vert_ram.count == 6144) {
              LOG_ERROR("GPU: submitted more vertices than fit into Vertex RAM.");
              break;
            }

            vert_ram.data[vert_ram.count++] = current_vertex_list[i];
          }

          is_first = false;
        }

        current_vertex_list.clear();
      }

      return;
    }

    if (needs_clipping) {
      // Keep the last two unclipped vertices to restart the polygon strip.
      if (is_strip) {
        next_vertex_list.push_back(current_vertex_list[current_vertex_list.size() - 2]);
        next_vertex_list.push_back(current_vertex_list[current_vertex_list.size() - 1]);
      }

      current_vertex_list = ClipPolygon(current_vertex_list, is_quad && is_strip);
    }

    for (auto const& v : current_vertex_list) {
      // TODO: can we do this more efficiently?
      if (vert_ram.count == 6144) {
        LOG_ERROR("GPU: submitted more vertices than fit into Vertex RAM.");
        break;
      }

      size_t index = vert_ram.count++;
      vert_ram.data[index] = v;
      poly.vertices[poly.count++] = &vert_ram.data[index];
    }

    // ClipPolygon() will have already swapped the vertices.
    if (!needs_clipping) {
      /**
       * v0---v2     v0---v3
       *  |    | -->  |    |
       * v1---v3     v1---v2
       */
      if (is_strip && is_quad) {
        std::swap(poly.vertices[2], poly.vertices[3]);
      }
    }

    // Setup state for the next polygon.
    if (needs_clipping && is_strip) {
      // Restart the polygon strip based on the last two submitted vertices.
      is_first = true;
      current_vertex_list = next_vertex_list;
    } else {
      is_first = false;
      current_vertex_list.clear();
    }

    if (is_strip) {
      polygon_strip_length++;
    }

    if (poly.count != 0) {
      poly_ram.count++;
      poly.params = poly_params;
      poly.texture_params = texture_params;

      bool has_translucent_polygon_alpha = poly.params.alpha != 0 && poly.params.alpha != 31;

      bool has_translucent_texture_format = poly.texture_params.format == TextureParams::Format::A3I5 ||
                                            poly.texture_params.format == TextureParams::Format::A5I3;

      bool uses_texture_alpha = poly.params.mode == PolygonParams::Mode::Modulation ||
                                poly.params.mode == PolygonParams::Mode::Shaded;

      poly.translucent = has_translucent_polygon_alpha || (has_translucent_texture_format && uses_texture_alpha);
    }
  }
}

bool GPU::IsFrontFacing(
  Vector4<Fixed20x12> const& v0,
  Vector4<Fixed20x12> const& v1,
  Vector4<Fixed20x12> const& v2,
  bool invert
) {
  float a[3] {
    (float)(v1.x() - v0.x()).raw() / (float)(1 << 12),
    (float)(v1.y() - v0.y()).raw() / (float)(1 << 12),
    (float)(v1.w() - v0.w()).raw() / (float)(1 << 12)
  };

  float b[3] {
    (float)(v2.x() - v0.x()).raw() / (float)(1 << 12),
    (float)(v2.y() - v0.y()).raw() / (float)(1 << 12),
    (float)(v2.w() - v0.w()).raw() / (float)(1 << 12)
  };

  float normal[3] {
    a[1] * b[2] - a[2] * b[1],
    a[2] * b[0] - a[0] * b[2],
    a[0] * b[1] - a[1] * b[0]
  };

  float dot = (v0.x().raw() / (float)(1 << 12)) * normal[0] +
              (v0.y().raw() / (float)(1 << 12)) * normal[1] +
              (v0.w().raw() / (float)(1 << 12)) * normal[2];

  if (invert) {
    return dot >= 0;
  }

  return dot < 0;
}

auto GPU::ClipPolygon(
  StaticVec<Vertex, 10> const& vertex_list,
  bool quadstrip
) -> StaticVec<Vertex, 10> {
  StaticVec<Vertex, 10> clipped[2];

  clipped[0] = vertex_list;

  if (quadstrip) {
    std::swap(clipped[0][2], clipped[0][3]);
  }

  struct CompareLt {
    bool operator()(Fixed20x12 x, Fixed20x12 w) { return x < -w; }
  };

  struct CompareGt {
    bool operator()(Fixed20x12 x, Fixed20x12 w) { return x >  w; }
  };

  if (!poly_params.render_far_plane_polys && ClipPolygonAgainstPlane<2, CompareGt>(clipped[0], clipped[1])) {
    return {};
  }
  clipped[0].clear();
  ClipPolygonAgainstPlane<2, CompareLt>(clipped[1], clipped[0]);
  clipped[1].clear();

  ClipPolygonAgainstPlane<1, CompareGt>(clipped[0], clipped[1]);
  clipped[0].clear();
  ClipPolygonAgainstPlane<1, CompareLt>(clipped[1], clipped[0]);
  clipped[1].clear();

  ClipPolygonAgainstPlane<0, CompareGt>(clipped[0], clipped[1]);
  clipped[0].clear();
  ClipPolygonAgainstPlane<0, CompareLt>(clipped[1], clipped[0]);
  // clipped[1].clear();

  return clipped[0];
}

template<int axis, typename Comparator>
bool GPU::ClipPolygonAgainstPlane(
  StaticVec<Vertex, 10> const& vertex_list_in,
  StaticVec<Vertex, 10>& vertex_list_out
) {
  const int precision = 18;

  auto size = (int)vertex_list_in.size();
  bool clipped = false;

  for (int i = 0; i < size; i++) {
    auto& v0 = vertex_list_in[i];

    if (Comparator{}(v0.position[axis], v0.position.w())) {
      int a = i - 1;
      int b = i + 1;
      if (a == -1) a = size - 1;
      if (b == size) b = 0;

      for (int j : { a, b }) {
        auto& v1 = vertex_list_in[j];

        if (!Comparator{}(v1.position[axis], v1.position.w())) {
          auto sign  = v0.position[axis] < -v0.position.w() ? 1 : -1;
          auto numer = v1.position[axis].raw() + sign * v1.position[3].raw();
          auto denom = (v0.position.w() - v1.position.w()).raw() + (v0.position[axis] - v1.position[axis]).raw() * sign;
          auto scale =  -sign * ((s64)numer << precision) / denom;
          auto scale_inv = (1 << precision) - scale;

          auto position = Vector4<Fixed20x12>{};
          auto color = Color4{};
          auto uv = Vector2<Fixed12x4>{};

          for (int k = 0; k < 4; k++) {
            position[k] = (v1.position[k].raw() * scale_inv + v0.position[k].raw() * scale) >> precision;
            color[k] = (v1.color[k].raw() * scale_inv + v0.color[k].raw() * scale) >> precision;
          }

          for (int k = 0; k < 2; k++) {
            uv[k] = (v1.uv[k].raw() * scale_inv + v0.uv[k].raw() * scale) >> precision;
          }

          vertex_list_out.push_back({position, color, uv});
        }
      }

      clipped = true;
    } else {
      vertex_list_out.push_back(v0);
    }
  }

  return clipped;
}

} // namespace lunar::nds
