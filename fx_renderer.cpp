#include "fx_renderer.h"

#include "fx_manager.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"

#include "sdl.h"

namespace fx {

static FXRenderer *s_instance = nullptr;

FXRenderer *FXRenderer::getInstance() { return s_instance; }

FXRenderer::FXRenderer(DirectoryPath dataPath, FXManager &mgr) : m_mgr(mgr) {
  m_textures.reserve(m_mgr.particleDefs().size() + 1);
  m_textureScales.reserve(m_mgr.particleDefs().size() + 1);
  m_textureIds.reserve(m_mgr.particleDefs().size());

  // First texture: invalid
  m_textures.emplace_back(Color::PINK, 2, 2);
  m_textureScales.emplace_back(FVec2(1.0f));

  // TODO: error handling
  for (auto &pdef : m_mgr.particleDefs()) {
    if (pdef.textureName.empty()) {
      m_textureIds.emplace_back(0);
      continue;
    }

    auto path = dataPath.file(pdef.textureName);
    int id = -1;
    for (int n = 0; n < (int)m_textures.size(); n++)
      if (m_textures[n].getPath() == path) {
        id = n;
        break;
      }

    if (id == -1) {
      id = m_textures.size();
      m_textures.emplace_back(path);
      auto tsize = m_textures.back().getSize(), rsize = m_textures.back().getRealSize();
      FVec2 scale(float(tsize.x) / float(rsize.x), float(tsize.y) / float(rsize.y));
      m_textureScales.emplace_back(scale);
    }
    m_textureIds.emplace_back(id);
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
  m_texCoords.clear();
  m_colors.clear();
  m_elements.clear();

  FVec2 foffset(offset.x, offset.y);
  FVec2 texScale(1);

  for (auto &quad : particles) {
    int texId = m_textureIds[quad.particleDefId];
    if (m_elements.empty() || m_elements.back().textureId != texId) {
      Element new_elem{(int)m_positions.size(), 0, texId};
      texScale = m_textureScales[texId];
      m_elements.emplace_back(new_elem);
    }
    m_elements.back().numVertices += 4;

    for (int n = 0; n < 4; n++) {
      FVec2 pos(quad.positions[n].x * zoom, quad.positions[n].y * zoom);
      pos = foffset + pos;

      m_positions.emplace_back(pos);
      m_texCoords.emplace_back(quad.texCoords[n].x * texScale.x, quad.texCoords[n].y * texScale.y);
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

  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glEnableClientState(GL_VERTEX_ARRAY);
  SDL::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glEnableClientState(GL_COLOR_ARRAY);

  SDL::glVertexPointer(2, GL_FLOAT, 0, m_positions.data());
  SDL::glTexCoordPointer(2, GL_FLOAT, 0, m_texCoords.data());
  SDL::glColorPointer(4, GL_UNSIGNED_BYTE, 0, m_colors.data());
  checkOpenglError();

  SDL::glEnable(GL_TEXTURE_2D);
  SDL::glDisable(GL_CULL_FACE);
  SDL::glDisable(GL_DEPTH_TEST);

  for (auto &elem : m_elements) {
    auto &tex = m_textures[elem.textureId];
    SDL::glBindTexture(GL_TEXTURE_2D, tex.getTexId());
    SDL::glDrawArrays(GL_QUADS, elem.firstVertex, elem.numVertices);
  }

  checkOpenglError();

  // TODO: check OpenGL errors ?
  SDL::glDisableClientState(GL_VERTEX_ARRAY);
  SDL::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glDisableClientState(GL_COLOR_ARRAY);
  SDL::glPopAttrib();
}
}
