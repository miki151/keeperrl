#include "fx_renderer.h"

#include "fx_manager.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"

#include "sdl.h"

namespace fx {

static FXRenderer *s_instance = nullptr;

FXRenderer *FXRenderer::getInstance() { return s_instance; }

FXRenderer::FXRenderer(DirectoryPath data_path, FXManager &mgr) : m_mgr(mgr) {
  auto pdir = data_path.subdirectory("images").subdirectory("particles");

  // TODO: error handling
  for(auto &pdef : m_mgr.particleDefs()) {
    if(pdef.texture_name.empty()) {
      m_textures.emplace_back(Color::PINK, 2, 2);
      continue;
    }

    m_textures.emplace_back(pdir.file(pdef.texture_name));
    auto tsize = m_textures.back().size, rsize = m_textures.back().realSize;
    FVec2 scale(float(tsize.x) / float(rsize.x), float(tsize.y) / float(rsize.y));
    m_texture_scales.emplace_back(scale);
  }
  CHECK(s_instance == nullptr && "There can be only one!");
  s_instance = this;
}

FXRenderer::~FXRenderer() { s_instance = nullptr; }

static void checkOpenglError() {
  auto error = SDL::glGetError();
  CHECK(error == GL_NO_ERROR) << (int)error;
}

void FXRenderer::draw(float zoom, Vec2 offset) {
  auto particles = m_mgr.genQuads();

  m_positions.clear();
  m_tex_coords.clear();
  m_colors.clear();
  m_elements.clear();

  FVec2 foffset(offset.x, offset.y);
  FVec2 tex_scale(1);

  for(auto &quad : particles) {
    if(m_elements.empty() || m_elements.back().texture_id != quad.particle_def_id) {
      Element new_elem{(int)m_positions.size(), 0, quad.particle_def_id};
      tex_scale = m_texture_scales[quad.particle_def_id];
      m_elements.emplace_back(new_elem);
    }
    m_elements.back().num_vertices += 4;

    for(int n = 0; n < 4; n++) {
      FVec2 pos(quad.positions[n].x * zoom, quad.positions[n].y * zoom);
      pos = foffset + pos;

      m_positions.emplace_back(pos);
      m_tex_coords.emplace_back(quad.tex_coords[n].x * tex_scale.x, quad.tex_coords[n].y * tex_scale.y);
      union {
        struct {
          unsigned char r, g, b, a;
        };
        unsigned int ivalue;
      };

      r = quad.color.r, g = quad.color.g, b = quad.color.b, a = quad.color.a;
      m_colors.emplace_back(ivalue);
    }
  }

  SDL::glEnableClientState(GL_VERTEX_ARRAY);
  SDL::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glEnableClientState(GL_COLOR_ARRAY);

  SDL::glVertexPointer(2, GL_FLOAT, 0, m_positions.data());
  SDL::glTexCoordPointer(2, GL_FLOAT, 0, m_tex_coords.data());
  SDL::glColorPointer(4, GL_UNSIGNED_BYTE, 0, m_colors.data());
  checkOpenglError();

  SDL::glEnable(GL_TEXTURE_2D);
  SDL::glDisable(GL_CULL_FACE);

  for(auto &elem : m_elements) {
    auto &tex = m_textures[elem.texture_id];
    PASSERT(tex.texId);

    SDL::glBindTexture(GL_TEXTURE_2D, *tex.texId);
    SDL::glDrawArrays(GL_QUADS, elem.first_vertex, elem.num_vertices);
  }

  checkOpenglError();

  // TODO: check OpenGL errors ?
  SDL::glDisableClientState(GL_VERTEX_ARRAY);
  SDL::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glDisableClientState(GL_COLOR_ARRAY);
  SDL::glDisable(GL_TEXTURE_2D);
}
}
