#include "fx_renderer.h"

#include "fx_manager.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"
#include "fx_draw_buffers.h"
#include "fx_rect.h"

#include "opengl.h"
#include "framebuffer.h"
#include "renderer.h"

namespace fx {

static constexpr int nominalSize = Renderer::nominalSize;

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

void FXRenderer::initFramebuffer(IVec2 size) {
  if (!Framebuffer::isExtensionAvailable())
    return;
  if (!m_framebuffer || m_framebuffer->width != size.x || m_framebuffer->height != size.y) {
    INFO << "FX: creating FBO (" << size.x << ", " << size.y << ")";
    m_framebuffer = std::make_unique<Framebuffer>(size.x, size.y);
  }
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

IRect FXRenderer::visibleTiles(const View& view) {
  float scale = 1.0f / (view.zoom * nominalSize);

  FVec2 topLeft = -view.offset * scale;
  FVec2 size = FVec2(view.size) * scale;

  IVec2 iTopLeft(floor(topLeft.x), floor(topLeft.y));
  IVec2 iSize(ceil(size.x), ceil(size.y));

  return IRect(iTopLeft - IVec2(1, 1), iTopLeft + iSize + IVec2(1, 1));
}

void FXRenderer::draw(float zoom, float offsetX, float offsetY, int w, int h) {
  View view{zoom, {offsetX, offsetY}, {w, h}};

  auto fboView = visibleTiles(view);
  auto fboScreenSize = fboView.size() * nominalSize;

  //print("FBO: (% % - % %)\n", fboView.x(), fboView.y(), fboView.ex(), fboView.ey());
  m_drawBuffers->fill(m_mgr.genQuads());
  applyTexScale();

  if (o_useFramebuffer) {
    // TODO: make it pixel perfect
    initFramebuffer(fboScreenSize);
  }

  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glDisable(GL_DEPTH_TEST);
  SDL::glDisable(GL_CULL_FACE);
  SDL::glEnable(GL_TEXTURE_2D);

  if (m_framebuffer && o_useFramebuffer) {
    m_framebuffer->bind();

    pushOpenglView();
    SDL::glPushAttrib(GL_ENABLE_BIT);
    SDL::glDisable(GL_SCISSOR_TEST);
    setupOpenglView(fboScreenSize.x, fboScreenSize.y, 1.0f);

    SDL::glClearColor(0.0f, 0.0f, 0.0f, 0.f);
    SDL::glClear(GL_COLOR_BUFFER_BIT);

    FVec2 fboOffset = -FVec2(fboView.min() * nominalSize);
    drawParticles({1.0f, fboOffset, fboScreenSize});

    // TODO: positioning is wrong for non-integral zoom values
    FVec2 c1 = FVec2(fboView.min() * nominalSize * zoom) + view.offset;
    FVec2 c2 = FVec2(fboView.max() * nominalSize * zoom) + view.offset;
    //print("fbo: % - %\n", fboView.min(), fboView.max());
    //print("c1: %   c2: %\n", c1, c2);

    Framebuffer::unbind();
    SDL::glPopAttrib();
    popOpenglView();

    SDL::glBlendFunc(GL_ONE, GL_ONE);
    SDL::glBindTexture(GL_TEXTURE_2D, m_framebuffer->texId);
    SDL::glBegin(GL_QUADS);
    SDL::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    SDL::glTexCoord2f(0.0f, 0.0f), SDL::glVertex2f(c1.x, c2.y);
    SDL::glTexCoord2f(1.0f, 0.0f), SDL::glVertex2f(c2.x, c2.y);
    SDL::glTexCoord2f(1.0f, 1.0f), SDL::glVertex2f(c2.x, c1.y);
    SDL::glTexCoord2f(0.0f, 1.0f), SDL::glVertex2f(c1.x, c1.y);
    SDL::glEnd();
  } else {
    drawParticles(view);
  }

  SDL::glPopAttrib();
  SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void FXRenderer::setBlendingMode(BlendMode bm) {
  if (m_framebuffer) {
    if (bm == BlendMode::normal)
      SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else
      SDL::glBlendFunc(GL_ONE, GL_ONE);
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
}
}
