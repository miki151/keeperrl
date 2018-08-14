#include "fx_renderer.h"

#include "fx_manager.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"
#include "fx_draw_buffers.h"
#include "fx_rect.h"

#include "opengl.h"
#include "framebuffer.h"

namespace fx {

struct FXRenderer::View {
  float zoom;
  FVec2 offset;
  IVec2 size;
};

static FXRenderer *s_instance = nullptr;

FXRenderer *FXRenderer::getInstance() { return s_instance; }

FXRenderer::FXRenderer(DirectoryPath dataPath, FXManager &mgr) : m_mgr(mgr) {
  m_textures.reserve(m_mgr.particleDefs().size() + 1);
  m_textureScales.reserve(m_mgr.particleDefs().size() + 1);
  m_textureIds.reserve(m_mgr.particleDefs().size());

  // First texture: invalid
  m_textures.emplace_back(Color::PINK, 2, 2);
  m_textureScales.emplace_back(FVec2(1.0f));

  m_drawBuffers = std::make_unique<DrawBuffers>();

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

void FXRenderer::initFramebuffer(int w, int h) {
  if (!Framebuffer::isExtensionAvailable())
    return;
  if (!m_framebuffer || m_framebuffer->width != w || m_framebuffer->height != h)
    m_framebuffer = std::make_unique<Framebuffer>(w, h);
}

void FXRenderer::applyTexScale() {
  auto& elements = m_drawBuffers->elements;
  auto& texCoords = m_drawBuffers->texCoords;

  for (auto& elem : elements) {
    int texId = m_textureIds[elem.particleDefId];
    auto scale = m_textureScales[texId];
    if (scale == FVec2(1.0f))
      continue;

    int end = elem.firstVertex + elem.numVertices;
    for (int i = elem.firstVertex; i < end; i++)
      texCoords[i] *= scale;
  }
}

// Rendering do framebuffera:
// - na początku wszystkie efekty do jednego bufora (zaczymamy z czarnym tłem?)
// - Problem: jak blendować cząsteczki z czarnym tłem ?texturew

IRect FXRenderer::framebufferView(const View& view) {
  return {};
}

void FXRenderer::draw(float zoom, float offsetX, float offsetY, int w, int h) {
  View view{zoom, {offsetX, offsetY}, {w, h}};
  auto fboView = framebufferView(view);
  //print("FBO: %\n", fboView);

  m_drawBuffers->fill(m_mgr.genQuads());
  applyTexScale();

  if (o_useFramebuffer) {
    // TODO: make it pixel perfect
    initFramebuffer(w / zoom, h / zoom);
  }

  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glDisable(GL_DEPTH_TEST);
  SDL::glDisable(GL_CULL_FACE);
  SDL::glEnable(GL_TEXTURE_2D);

  if (m_framebuffer && o_useFramebuffer) {
    m_framebuffer->bind();

    SDL::glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    SDL::glViewport(0, 0, m_framebuffer->width, m_framebuffer->height);
    SDL::glDisable(GL_SCISSOR_TEST);

    SDL::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    SDL::glClear(GL_COLOR_BUFFER_BIT);

    drawParticles(view);
    Framebuffer::unbind();
    SDL::glPopAttrib();

    SDL::glBlendFunc(GL_ONE, GL_ONE);

    SDL::glBindTexture(GL_TEXTURE_2D, m_framebuffer->texId);
    SDL::glBegin(GL_QUADS);
    SDL::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    SDL::glTexCoord2f(0.0f, 0.0f);
    SDL::glVertex2i(50, h);
    SDL::glTexCoord2f(1.0f, 0.0f);
    SDL::glVertex2i(w - 50, h);
    SDL::glTexCoord2f(1.0f, 1.0f);
    SDL::glVertex2i(w - 50, 0);
    SDL::glTexCoord2f(0.0f, 1.0f);
    SDL::glVertex2i(50, 0);
    SDL::glEnd();
    checkOpenglError();
  } else {
    drawParticles(view);
  }

  SDL::glPopAttrib();
}

void FXRenderer::setBlendingMode(BlendMode bm) {
  if (m_framebuffer) {
    if (bm == BlendMode::normal)
      SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else
      SDL::glBlendFunc(GL_ONE, GL_DST_ALPHA);
    return;
  }

  if (bm == BlendMode::normal)
    SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  else
    SDL::glBlendFunc(GL_ONE, GL_ONE);
}

void FXRenderer::drawParticles(const View& view) {
  SDL::glPushMatrix();

  SDL::glTranslatef(view.offset.x, view.offset.y, 0.0f);
  SDL::glScalef(view.zoom, view.zoom, 1.0f);

  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glEnableClientState(GL_VERTEX_ARRAY);
  SDL::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glEnableClientState(GL_COLOR_ARRAY);

  SDL::glVertexPointer(2, GL_FLOAT, 0, m_drawBuffers->positions.data());
  SDL::glTexCoordPointer(2, GL_FLOAT, 0, m_drawBuffers->texCoords.data());
  SDL::glColorPointer(4, GL_UNSIGNED_BYTE, 0, m_drawBuffers->colors.data());
  checkOpenglError();

  auto bm = BlendMode::normal;
  setBlendingMode(bm);

  for (auto& elem : m_drawBuffers->elements) {
    int texId = m_textureIds[elem.particleDefId];
    auto& tex = m_textures[texId];
    if (elem.blendMode != bm) {
      bm = elem.blendMode;
      setBlendingMode(bm);
    }
    SDL::glBindTexture(GL_TEXTURE_2D, *tex.getTexId());
    SDL::glDrawArrays(GL_QUADS, elem.firstVertex, elem.numVertices);
  }

  checkOpenglError();

  // TODO: check OpenGL errors ?
  SDL::glDisableClientState(GL_VERTEX_ARRAY);
  SDL::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glDisableClientState(GL_COLOR_ARRAY);
  SDL::glPopAttrib();
  SDL::glPopMatrix();
  SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
}
