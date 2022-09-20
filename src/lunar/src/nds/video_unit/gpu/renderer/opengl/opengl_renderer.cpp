/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <optional>

#include "shader/edge_marking.glsl.hpp"
#include "shader/test.glsl.hpp"
#include "opengl_renderer.hpp"

GLuint opengl_color_texture;

namespace lunar::nds {

OpenGLRenderer::OpenGLRenderer(
  Region<4, 131072> const& vram_texture,
  Region<8> const& vram_palette,
  GPU::DISP3DCNT const& disp3dcnt,
  GPU::AlphaTest const& alpha_test
)   : texture_cache{vram_texture, vram_palette}, disp3dcnt{disp3dcnt}, alpha_test{alpha_test} {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_BLEND);

  // TODO: abstract FBO creation
  {
    constexpr GLenum k_draw_buffers[2] {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    color_texture = Texture2D::Create(512, 384, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
    opaque_poly_id_texture = Texture2D::Create(512, 384, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
    depth_texture = Texture2D::Create(512, 384, GL_DEPTH_STENCIL, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
    opengl_color_texture = color_texture->Handle(); // @hack: for reading in the frontend

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture->Handle(), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, opaque_poly_id_texture->Handle(), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_texture->Handle(), 0);
    glDrawBuffers(2, k_draw_buffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  program = ProgramObject::Create(test_vert, test_frag);
  program->SetUniformInt("u_map", 0);
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

  program_edge_marking = ProgramObject::Create(edge_marking_vert, edge_marking_frag);
  program_edge_marking->SetUniformInt("u_color_map", 0);
  program_edge_marking->SetUniformInt("u_depth_map", 1);
  program_edge_marking->SetUniformInt("u_opaque_poly_id_map", 2);
  quad_vao = VertexArrayObject::Create();
  quad_vbo = BufferObject::CreateArrayBuffer(sizeof(k_quad_vertices), GL_STATIC_DRAW);
  quad_vao->SetAttribute(0, VertexArrayObject::Attribute{quad_vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0});
  quad_vao->SetAttribute(1, VertexArrayObject::Attribute{quad_vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float)});
  quad_vbo->Upload(k_quad_vertices, sizeof(k_quad_vertices) / sizeof(float));
}

OpenGLRenderer::~OpenGLRenderer() {
  {
    glDeleteFramebuffers(1, &fbo);
    delete color_texture;
    delete opaque_poly_id_texture;
    delete depth_texture;
  }

  delete program;
  delete vao;
  delete vbo;

  delete program_edge_marking;
  delete quad_vao;
  delete quad_vbo;
}

void OpenGLRenderer::Render(void const* polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const*)polygons_;

  SetupAndUploadVBO(polygons, polygon_count);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glViewport(0, 0, 512, 384);
  RenderRearPlane();
  RenderPolygons(polygons, polygon_count, false);
  RenderPolygons(polygons, polygon_count, true);
  RenderEdgeMarking();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::RenderRearPlane() {
  const float clear_color_0[4] {0, 0, 0, 1};
  const float clear_color_1[4] {0, 0, 0, 0};

  // @todo: replace this stub with a real implementation
  glDepthMask(GL_TRUE);
  glStencilMask(0xFF); // should this be zero or 0xFF?
  glClearBufferfv(GL_COLOR, 0, clear_color_0);
  glClearBufferfv(GL_COLOR, 1, clear_color_1);
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void OpenGLRenderer::RenderPolygons(void const* polygons_, int polygon_count, bool translucent) {
  using Format = GPU::TextureParams::Format;

  auto polygons = (GPU::Polygon const*)polygons_;

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
    auto& polygon = polygons[i];

    int vertices = (polygon.count - 2) * 3;

    RenderState current_state{};
    current_state.texture_params = &polygon.texture_params;
    current_state.alpha = polygon.params.alpha;
    current_state.enable_translucent_depth_write = polygon.params.enable_translucent_depth_write;
    current_state.polygon_id = polygon.params.polygon_id;
    current_state.polygon_mode = (int)polygon.params.mode;
    current_state.depth_test = (int)polygon.params.depth_test;

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

  program->Use();
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

    if (translucent == (alpha != 31)) {
      auto texture_params = (GPU::TextureParams const*)batch.state.texture_params;

      auto format = texture_params->format;
      bool use_map = disp3dcnt.enable_textures && format != GPU::TextureParams::Format::None;
      bool use_alpha_test = disp3dcnt.enable_alpha_test;

      program->SetUniformBool("u_use_map", use_map);

      if(use_map) {
        texture_cache.Get(texture_params)->Bind(GL_TEXTURE0);

        // update alpha-test uniforms
        program->SetUniformBool("u_use_alpha_test", use_alpha_test);
        program->SetUniformFloat("u_alpha_test_threshold", (float)alpha_test.alpha / 31.0f);
      }

      program->SetUniformFloat("u_polygon_alpha", (float)alpha / 31.0f);
      program->SetUniformFloat("u_polygon_id", (float)batch.state.polygon_id / 31.0f);

      if (batch.state.depth_test == (int)GPU::PolygonParams::DepthTest::Less) {
        // @todo: use GL_LESS. this might require extra care with the per-pixel depth update logic though.
        glDepthFunc(GL_LEQUAL);
      } else {
        glDepthFunc(GL_EQUAL);
      }

      glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

      if(batch.state.polygon_mode == (int) GPU::PolygonParams::Mode::Shadow) {
        if (batch.state.polygon_id == 0) {
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
          glStencilFunc(GL_EQUAL, STENCIL_FLAG_SHADOW | batch.state.polygon_id, STENCIL_FLAG_SHADOW | STENCIL_MASK_POLY_ID);
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
        glStencilFunc(GL_ALWAYS, batch.state.polygon_id, 0);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

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
  }

  glUseProgram(0);
}

void OpenGLRenderer::RenderEdgeMarking() {
  // TODO: do not commit sins like this:
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDepthFunc(GL_ALWAYS);

  program_edge_marking->Use();
  color_texture->Bind(GL_TEXTURE0);
  depth_texture->Bind(GL_TEXTURE1);
  opaque_poly_id_texture->Bind(GL_TEXTURE2);
  quad_vao->Bind();
  glDrawArrays(GL_QUADS, 0, 4);

  glUseProgram(0);
}

void OpenGLRenderer::SetupAndUploadVBO(void const* polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const*)polygons_;

  vertex_buffer.clear();

  for (int i = 0; i < polygon_count; i++) {
    auto& polygon = polygons[i];

    for (int j = 1; j < polygon.count - 1; j++) {
      for (int k : {0, j, j + 1}) {
        auto vert = polygon.vertices[k];

        float position_x = (float)vert->position.x().raw() / (float)(1 << 12);
        float position_y = (float)vert->position.y().raw() / (float)(1 << 12);
        float position_z = (float)vert->position.z().raw() / (float)(1 << 12);
        float position_w = (float)vert->position.w().raw() / (float)(1 << 12);

        float color_a = (float)vert->color.a().raw() / (float)(1 << 6);
        float color_r = (float)vert->color.r().raw() / (float)(1 << 6);
        float color_g = (float)vert->color.g().raw() / (float)(1 << 6);
        float color_b = (float)vert->color.b().raw() / (float)(1 << 6);

        float texcoord_s = (float)vert->uv.x().raw() / (float)(1 << 4);
        float texcoord_t = (float)vert->uv.y().raw() / (float)(1 << 4);

        vertex_buffer.push_back({position_x, position_y, position_z, position_w, color_r, color_g, color_b, color_a, texcoord_s, texcoord_t});
      }
    }
  }

  vbo->Upload(vertex_buffer.data(), vertex_buffer.size());
}

bool OpenGLRenderer::RenderState::operator==(OpenGLRenderer::RenderState const& other) const {
  // @todo: move this back into the header file, once the GPU definition thing is fixed.
  return ((GPU::TextureParams*)texture_params)->raw_value == ((GPU::TextureParams*)other.texture_params)->raw_value &&
         alpha == other.alpha &&
         enable_translucent_depth_write == other.enable_translucent_depth_write &&
         polygon_id == other.polygon_id &&
         polygon_mode == other.polygon_mode &&
         depth_test == other.depth_test;
}

} // namespace lunar::nds