/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <optional>

#include "shader/edge_marking.glsl.hpp"
#include "shader/fog.glsl.hpp"
#include "shader/geometry.glsl.hpp"
#include "opengl_renderer.hpp"

namespace lunar::nds {

OpenGLRenderer::OpenGLRenderer(
  Region<4, 131072> const& vram_texture,
  Region<8> const& vram_palette,
  GPU::DISP3DCNT const& disp3dcnt,
  GPU::AlphaTest const& alpha_test,
  GPU::ClearColor const& clear_color,
  GPU::ClearDepth const& clear_depth,
  GPU::FogColor const& fog_color,
  GPU::FogOffset const& fog_offset,
  std::array<u16, 8> const& edge_color_table
)   : texture_cache{vram_texture, vram_palette}
    , disp3dcnt{disp3dcnt}
    , alpha_test{alpha_test}
    , clear_color(clear_color)
    , clear_depth(clear_depth)
    , fog_color{fog_color}
    , fog_offset{fog_offset}
    , edge_color_table{edge_color_table} {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_BLEND);

  {
    color_texture = Texture2D::Create(512, 384, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
    attribute_texture = Texture2D::Create(512, 384, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
    depth_texture = Texture2D::Create(512, 384, GL_DEPTH_STENCIL, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);

    fbo = FrameBufferObject::Create();
    fbo->Attach(GL_COLOR_ATTACHMENT0, color_texture);
    fbo->Attach(GL_COLOR_ATTACHMENT1, attribute_texture);
    fbo->Attach(GL_DEPTH_STENCIL_ATTACHMENT, depth_texture);
    fbo->DrawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
  }

  program = ProgramObject::Create(geometry_vert, geometry_frag);
  program->SetUniformInt("u_map", 0);
  program->SetUniformInt("u_toon_table", 1);
  vbo = BufferObject::CreateArrayBuffer(sizeof(BufferVertex) * k_total_vertices, GL_DYNAMIC_DRAW);
  vao = VertexArrayObject::Create();
  vao->SetAttribute(0, VertexArrayObject::Attribute{vbo, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 0});
  vao->SetAttribute(1, VertexArrayObject::Attribute{vbo, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 4 * sizeof(float)});
  vao->SetAttribute(2, VertexArrayObject::Attribute{vbo, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 8 * sizeof(float)});

  static constexpr float k_quad_vertices[] {
    // POSITION | UV
    -1.0, -1.0,    0.0, 0.0,
     1.0, -1.0,    1.0, 0.0,
     1.0,  1.0,    1.0, 1.0,
    -1.0,  1.0,    0.0, 1.0
  };

  fbo_edge_marking = FrameBufferObject::Create();
  fbo_edge_marking->Attach(GL_COLOR_ATTACHMENT0, color_texture);
  program_edge_marking = ProgramObject::Create(edge_marking_vert, edge_marking_frag);
  program_edge_marking->SetUniformInt("u_depth_map", 0);
  program_edge_marking->SetUniformInt("u_attribute_map", 1);
  quad_vao = VertexArrayObject::Create();
  quad_vbo = BufferObject::CreateArrayBuffer(sizeof(k_quad_vertices), GL_STATIC_DRAW);
  quad_vao->SetAttribute(0, VertexArrayObject::Attribute{quad_vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0});
  quad_vao->SetAttribute(1, VertexArrayObject::Attribute{quad_vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float)});
  quad_vbo->Upload(k_quad_vertices, sizeof(k_quad_vertices) / sizeof(float));

  fbo_fog = FrameBufferObject::Create();
  fog_output_texture = Texture2D::Create(512, 384, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
  fbo_fog->Attach(GL_COLOR_ATTACHMENT0, fog_output_texture);
  program_fog = ProgramObject::Create(fog_vert, fog_frag);
  program_fog->SetUniformInt("u_color_map", 0);
  program_fog->SetUniformInt("u_depth_map", 1);
  program_fog->SetUniformInt("u_attribute_map", 2);
  program_fog->SetUniformInt("u_fog_density_table", 3);
  fog_density_table_texture = Texture2D::Create(32, 1, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
  fog_density_table_texture->SetMagFilter(GL_LINEAR);
  fog_density_table_texture->SetWrapBehaviour(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

  toon_table_texture = Texture2D::Create(32, 1, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);

  final_output_texture = color_texture;
}

OpenGLRenderer::~OpenGLRenderer() {
  delete fbo;
  delete color_texture;
  delete attribute_texture;
  delete depth_texture;

  delete program;
  delete vao;
  delete vbo;

  delete fbo_edge_marking;
  delete program_edge_marking;
  delete quad_vao;
  delete quad_vbo;

  delete fbo_fog;
  delete fog_output_texture;
  delete program_fog;
  delete fog_density_table_texture;

  delete toon_table_texture;
}

void OpenGLRenderer::Render(void const** polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const**)polygons_;

  SetupAndUploadVBO(polygons_, polygon_count);

  fbo->Bind();

  glViewport(0, 0, 512, 384);
  RenderRearPlane();
  RenderPolygons(polygons_, polygon_count);

  if (disp3dcnt.enable_edge_marking) {
    RenderEdgeMarking();
  }

  if (disp3dcnt.enable_fog) {
    RenderFog();
    final_output_texture = fog_output_texture;
  } else {
    final_output_texture = color_texture;
  }

  fbo->Unbind();

  captured_current_frame = false;
}

void OpenGLRenderer::UpdateToonTable(std::array<u16, 32> const& toon_table) {
  u32 rgba[32];

  for (int i = 0; i < 32; i++) {
    u16 bgr555 = toon_table[i];

    // @todo: deduplicate this logic across the codebase
    int r = (bgr555 >>  0) & 31;
    int g = (bgr555 >>  5) & 31;
    int b = (bgr555 >> 10) & 31;

    rgba[i] = 0xFF000000 | r << 19 | g << 11 | b << 3;
  }

  toon_table_texture->Upload(rgba);
}

void OpenGLRenderer::UpdateFogDensityTable(std::array<u8, 32> const& fog_density_table) {
  u8 data[32];

  for (int i = 0; i < 32; i++) {
    data[i] = (fog_density_table[i] << 1) | (fog_density_table[i] >> 6);
  }

  fog_density_table_texture->Upload(data);
}

void OpenGLRenderer::SetWBufferEnable(bool enable) {
  use_w_buffer = enable;
}

void OpenGLRenderer::RenderRearPlane() {
  const float clear_color_0[4] {
    (float)clear_color.color_r / 31,
    (float)clear_color.color_g / 31,
    (float)clear_color.color_b / 31,
    (float)clear_color.color_a / 31
  };

  const float clear_color_1[4] {
    (float)clear_color.polygon_id / 63,
    clear_color.enable_fog ? 1.0f : 0.0f,
    0,
    0
  };

  glClearBufferfv(GL_COLOR, 0, clear_color_0);
  glClearBufferfv(GL_COLOR, 1, clear_color_1);

  glDepthMask(GL_TRUE);
  glStencilMask(0xFF);
  glClearDepth((float)clear_depth.depth / 0x7FFF);
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glClearDepth(1.0);
}

void OpenGLRenderer::RenderPolygons(void const** polygons_, int polygon_count) {
  using Format = GPU::TextureParams::Format;

  auto polygons = (GPU::Polygon const**)polygons_;

  // @todo: could use different VBOs for opaque and translucent.

  Batch current_batch;
  int next_batch_start = 0;

  batch_list.clear();

  const auto FinalizeBatch = [&]() {
    current_batch.vertex_count = next_batch_start - current_batch.vertex_start;
    if (current_batch.vertex_count != 0) {
      batch_list.push_back(current_batch);
    }
  };

  for (int i = 0; i < polygon_count; i++) {
    auto polygon = polygons[i];

    int vertices = (polygon->count - 2) * 3;

    RenderState current_state{};
    current_state.texture_params = &polygon->texture_params;
    current_state.alpha = polygon->params.alpha;
    current_state.enable_translucent_depth_write = polygon->params.enable_translucent_depth_write;
    current_state.polygon_id = polygon->params.polygon_id;
    current_state.polygon_mode = (int)polygon->params.mode;
    current_state.depth_test = (int)polygon->params.depth_test;
    current_state.fog = polygon->params.enable_fog;
    current_state.translucent = polygon->translucent;

    // make sure current batch state is initialized
    if (i == 0) {
      current_batch.state = current_state;
    }

    if (current_state != current_batch.state) {
      FinalizeBatch();
      current_batch = {current_state, next_batch_start, 0};
    }

    next_batch_start += vertices;
  }

  FinalizeBatch();

  // --------------------------------------------------

  // @todo: render batches as we build them, instead of buffering them first.
  // this should improve the GPU utilization, as rendering and batch generation can happen in parallel.

  program->Use();
  program->SetUniformInt("u_shading_mode", (int)disp3dcnt.shading_mode);
  program->SetUniformBool("u_use_w_buffer", use_w_buffer);
  vao->Bind();

  if (disp3dcnt.enable_alpha_blend) {
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
  } else {
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
  }

  for (auto const& batch : batch_list) {
    int alpha = batch.state.alpha;
    bool wireframe = false;

    if (alpha == 0) {
      wireframe = true;
      alpha = 31;
    }

    auto texture_params = (GPU::TextureParams const*)batch.state.texture_params;

    auto format = texture_params->format;
    bool use_map = disp3dcnt.enable_textures && format != GPU::TextureParams::Format::None;
    bool use_alpha_test = disp3dcnt.enable_alpha_test;
    int polygon_id = batch.state.polygon_id;
    int polygon_mode = batch.state.polygon_mode;

    program->SetUniformBool("u_use_map", use_map);

    if(use_map) {
      texture_cache.Get(texture_params)->Bind(GL_TEXTURE0);

      // update alpha-test uniforms
      program->SetUniformBool("u_use_alpha_test", use_alpha_test);
      program->SetUniformFloat("u_alpha_test_threshold", (float)alpha_test.alpha / 31.0f);
    }

    toon_table_texture->Bind(GL_TEXTURE1);

    program->SetUniformFloat("u_polygon_alpha", (float)alpha / 31.0f);
    program->SetUniformFloat("u_polygon_id", (float)polygon_id / 63.0f);
    program->SetUniformInt("u_polygon_mode", polygon_mode);
    program->SetUniformFloat("u_fog_flag", batch.state.fog ? 1.0 : 0.0);

    if(batch.state.depth_test == (int)GPU::PolygonParams::DepthTest::Less) {
      // @todo: use GL_LESS. this might require extra care with the per-pixel depth update logic though.
      glDepthFunc(GL_LEQUAL);
    } else {
      glDepthFunc(GL_EQUAL);
    }

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    if(polygon_mode == (int)GPU::PolygonParams::Mode::Shadow) {
      if (polygon_id == 0) {
        // @todo: should the depth buffer be updated if alpha==63 (or when translucent depth write is active)
        glDepthMask(GL_FALSE);
        glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        // Set STENCIL_FLAG_SHADOW bit for pixels which fail the depth-buffer test.
        glStencilMask(STENCIL_FLAG_SHADOW);
        glStencilFunc(GL_ALWAYS, STENCIL_FLAG_SHADOW, 0);
        glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP);

        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);

        glDepthMask(GL_TRUE);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      } else {
        // @todo: should the depth buffer be updated if alpha==63 (or when translucent depth write is active)
        glDepthMask(GL_FALSE);
        glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        // Reset the shadow flag for pixels where the polygon ID matches the one already in the stencil buffer.
        glStencilMask(STENCIL_FLAG_SHADOW);
        glStencilFunc(GL_EQUAL, STENCIL_FLAG_SHADOW | polygon_id, STENCIL_FLAG_SHADOW | STENCIL_MASK_POLY_ID);
        glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
        glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // Draw the shadow polygon pixels where the shadow flag is still set.
        glStencilMask(0);
        glStencilFunc(GL_EQUAL, STENCIL_FLAG_SHADOW, STENCIL_FLAG_SHADOW);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);

        glDepthMask(GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }
    } else {
      // Write polygon ID to the stencil buffer and reset the shadow flag
      glStencilMask(STENCIL_FLAG_SHADOW | STENCIL_MASK_POLY_ID),
      glStencilFunc(GL_ALWAYS, polygon_id, 0);
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

      // @todo: try to simplify this by checking batch.state.translucent
      if(batch.state.enable_translucent_depth_write || (
        alpha == 31 && !texture_params->color0_transparent && (
          format == Format::Palette2BPP ||
          format == Format::Palette4BPP ||
          format == Format::Palette8BPP))) {
        glDepthMask(GL_TRUE);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);
      } else {
        // @todo: make sure that the depth test does not break

        // render opaque pixels to the depth-buffer only first.
        program->SetUniformBool("u_discard_translucent_pixels", true);
        glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_TRUE);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // then render both opaque and translucent pixels without updating the depth buffer.
        program->SetUniformBool("u_discard_translucent_pixels", false);
        glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }
    }
  }

  glUseProgram(0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLRenderer::RenderEdgeMarking() {
  float edge_colors[24];

  // @todo: do not update if nothing has changed
  for (int i = 0; i < 8; i++) {
    u16 color = edge_color_table[i];

    float r = (float)((color >>  0) & 0x1F) / 31.0f;
    float g = (float)((color >>  5) & 0x1F) / 31.0f;
    float b = (float)((color >> 10) & 0x1F) / 31.0f;

    edge_colors[i * 3 + 0] = r;
    edge_colors[i * 3 + 1] = g;
    edge_colors[i * 3 + 2] = b;
  }

  program_edge_marking->SetUniformVec3Array("u_edge_colors", edge_colors, 8);

  // @todo: do we need to disable edge-marking for pixels covered by translucent polygons?
  program_edge_marking->Use();
  depth_texture->Bind(GL_TEXTURE0);
  attribute_texture->Bind(GL_TEXTURE1);

  fbo_edge_marking->Bind();
  quad_vao->Bind();
  glDrawArrays(GL_QUADS, 0, 4);
  glUseProgram(0);
}

void OpenGLRenderer::RenderFog() {
  float offset = (float)fog_offset.value / (float)0x8000;
  float width = 32 * (float)(0x400 >> disp3dcnt.fog_depth_shift) / (float)0x8000;

  float color_r = (float)fog_color.r / 31.0f;
  float color_g = (float)fog_color.g / 31.0f;
  float color_b = (float)fog_color.b / 31.0f;
  float color_a = (float)fog_color.a / 31.0f;

  program_fog->Use();
  program_fog->SetUniformFloat("u_fog_offset", offset);
  program_fog->SetUniformFloat("u_fog_width", width);
  program_fog->SetUniformVec4("u_fog_color", color_r, color_g, color_b, color_a);

  if(disp3dcnt.fog_mode == GPU::DISP3DCNT::FogMode::ColorAndAlpha) {
    program_fog->SetUniformVec4("u_fog_blend_mask", 1.0, 1.0, 1.0, 1.0);
  } else {
    program_fog->SetUniformVec4("u_fog_blend_mask", 0.0, 0.0, 0.0, 1.0);
  }

  color_texture->Bind(GL_TEXTURE0);
  depth_texture->Bind(GL_TEXTURE1);
  attribute_texture->Bind(GL_TEXTURE2);
  fog_density_table_texture->Bind(GL_TEXTURE3);

  fbo_fog->Bind();
  quad_vao->Bind();
  glDrawArrays(GL_QUADS, 0, 4);
  glUseProgram(0);
}

void OpenGLRenderer::SetupAndUploadVBO(void const** polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const**)polygons_;

  vertex_buffer.clear();

  for (int i = 0; i < polygon_count; i++) {
    auto& polygon = polygons[i];

    for (int j = 1; j < polygon->count - 1; j++) {
      for (int k : {0, j, j + 1}) {
        auto vert = polygon->vertices[k];

        float position_x =  (float)vert->position.X().raw() / (float)(1 << 12);
        float position_y = -(float)vert->position.Y().raw() / (float)(1 << 12);
        float position_z =  (float)vert->position.Z().raw() / (float)(1 << 12);
        float position_w =  (float)vert->position.W().raw() / (float)(1 << 12);

        float color_a = (float)vert->color.A().raw() / (float)(1 << 6);
        float color_r = (float)vert->color.R().raw() / (float)(1 << 6);
        float color_g = (float)vert->color.G().raw() / (float)(1 << 6);
        float color_b = (float)vert->color.B().raw() / (float)(1 << 6);

        float texcoord_s = (float)vert->uv.X().raw() / (float)(1 << 4);
        float texcoord_t = (float)vert->uv.Y().raw() / (float)(1 << 4);

        vertex_buffer.push_back({position_x, position_y, position_z, position_w, color_r, color_g, color_b, color_a, texcoord_s, texcoord_t});
      }
    }
  }

  vbo->Upload(vertex_buffer.data(), vertex_buffer.size());
}

void OpenGLRenderer::DoCapture() {
  if(!captured_current_frame) {
    if(disp3dcnt.enable_fog) {
      fbo_fog->Bind();
    } else {
      fbo->Bind();
    }

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, 512, 384, GL_BGRA, GL_UNSIGNED_BYTE, capture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    captured_current_frame = true;
  }
}

void OpenGLRenderer::CaptureColor(u16* buffer, int vcount, int width, bool display_capture) {
  DoCapture();

  // @todo: support different upscale factors.
  for(int x = 0; x < width; x++) {
    u32 argb8888 = capture[(vcount * 512 + x) * 2];

    uint a = (argb8888 >> 24) & 0xFF;
    uint r = (argb8888 >> 16) & 0xFF;
    uint g = (argb8888 >>  8) & 0xFF;
    uint b = (argb8888 >>  0) & 0xFF;

    u16 rgb555 = r >> 3 | g >> 3 << 5 | b >> 3 << 10;

    if (display_capture) {
      buffer[x] = rgb555 | (a != 0 ? 0x8000 : 0);
    } else {
      buffer[x] = a == 0 ? 0x8000 : rgb555;
    }
  }
}

void OpenGLRenderer::CaptureAlpha(int* buffer, int vcount) {
  DoCapture();

  // @todo: support different upscale factors.
  for (int x = 0; x < 256; x++) {
    buffer[x] = (int)(capture[(vcount * 512 + x) * 2] >> 28);
  }
}

bool OpenGLRenderer::RenderState::operator==(OpenGLRenderer::RenderState const& other) const {
  return ((GPU::TextureParams*)texture_params)->raw_value == ((GPU::TextureParams*)other.texture_params)->raw_value &&
         ((GPU::TextureParams*)texture_params)->palette_base == ((GPU::TextureParams*)other.texture_params)->palette_base &&
         alpha == other.alpha &&
         enable_translucent_depth_write == other.enable_translucent_depth_write &&
         polygon_id == other.polygon_id &&
         polygon_mode == other.polygon_mode &&
         depth_test == other.depth_test &&
         fog == other.fog;
}

} // namespace lunar::nds