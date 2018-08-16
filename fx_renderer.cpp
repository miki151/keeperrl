#include "fx_renderer.h"

#include "fx_manager.h"
#include "fx_defs.h"
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

FXRenderer::FXRenderer(DirectoryPath dataPath, FXManager& mgr) : mgr(mgr) {
  textures.reserve(mgr.particleDefs().size());
  textureScales.reserve(mgr.particleDefs().size());
  textureIds.reserve(mgr.particleDefs().size());
  drawBuffers = std::make_unique<DrawBuffers>();

  for (auto& pdef : mgr.particleDefs()) {
    auto path = dataPath.file(pdef.texture.name);
    int id = -1;
    for (int n = 0; n < (int)textures.size(); n++)
      if (textures[n].getPath() == path) {
        id = n;
        break;
      }

    if (id == -1) {
      id = textures.size();
      textures.emplace_back(path);
      auto tsize = textures.back().getSize(), rsize = textures.back().getRealSize();
      FVec2 scale(float(tsize.x) / float(rsize.x), float(tsize.y) / float(rsize.y));
      textureScales.emplace_back(scale);
    }
    textureIds.emplace_back(id);
  }
  CHECK(s_instance == nullptr && "There can be only one!");
  s_instance = this;
}

FXRenderer::~FXRenderer() { s_instance = nullptr; }

void FXRenderer::initFramebuffer(IVec2 size) {
  if (!Framebuffer::isExtensionAvailable())
    return;
  if (!framebuffer || framebuffer->width != size.x || framebuffer->height != size.y) {
    INFO << "FX: creating FBO (" << size.x << ", " << size.y << ")";
    framebuffer = std::make_unique<Framebuffer>(size.x, size.y);
  }
}

void FXRenderer::applyTexScale() {
  auto& elements = drawBuffers->elements;
  auto& texCoords = drawBuffers->texCoords;

  for (auto& elem : elements) {
    int texId = textureIds[elem.particleDefId];
    auto scale = textureScales[texId];
    if (scale == FVec2(1.0f))
      continue;

    int end = elem.firstVertex + elem.numVertices;
    for (int i = elem.firstVertex; i < end; i++)
      texCoords[i] *= scale;
  }
}

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

  drawBuffers->fill(mgr.genQuads());
  applyTexScale();

  if (useFramebuffer)
    initFramebuffer(fboScreenSize);

  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glDisable(GL_DEPTH_TEST);
  SDL::glDisable(GL_CULL_FACE);
  SDL::glEnable(GL_TEXTURE_2D);

  // Problem: FBO mode doesn't work well for particles blender normally
  if (framebuffer && useFramebuffer) {
    framebuffer->bind();

    pushOpenglView();
    SDL::glPushAttrib(GL_ENABLE_BIT);
    SDL::glDisable(GL_SCISSOR_TEST);
    setupOpenglView(fboScreenSize.x, fboScreenSize.y, 1.0f);

    SDL::glClearColor(0.0f, 0.0f, 0.0f, 0.f);
    SDL::glClear(GL_COLOR_BUFFER_BIT);

    FVec2 fboOffset = -FVec2(fboView.min() * nominalSize);
    drawParticles({1.0f, fboOffset, fboScreenSize}, BlendMode::additive);

    // TODO: positioning is wrong for non-integral zoom values
    FVec2 c1 = FVec2(fboView.min() * nominalSize * zoom) + view.offset;
    FVec2 c2 = FVec2(fboView.max() * nominalSize * zoom) + view.offset;

    Framebuffer::unbind();
    SDL::glPopAttrib();
    popOpenglView();

    SDL::glBlendFunc(GL_ONE, GL_ONE);
    SDL::glBindTexture(GL_TEXTURE_2D, framebuffer->texId);
    SDL::glBegin(GL_QUADS);
    SDL::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    SDL::glTexCoord2f(0.0f, 0.0f), SDL::glVertex2f(c1.x, c2.y);
    SDL::glTexCoord2f(1.0f, 0.0f), SDL::glVertex2f(c2.x, c2.y);
    SDL::glTexCoord2f(1.0f, 1.0f), SDL::glVertex2f(c2.x, c1.y);
    SDL::glTexCoord2f(0.0f, 1.0f), SDL::glVertex2f(c1.x, c1.y);
    SDL::glEnd();
  } else {
    drawParticles(view, BlendMode::additive);
  }

  drawParticles(view, BlendMode::normal);

  SDL::glPopAttrib();
  SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void FXRenderer::setBlendingMode(BlendMode bm) {
  if (framebuffer) {
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

void FXRenderer::drawParticles(const View& view, BlendMode blendMode) {
  SDL::glPushMatrix();

  SDL::glTranslatef(view.offset.x, view.offset.y, 0.0f);
  SDL::glScalef(view.zoom, view.zoom, 1.0f);

  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glEnableClientState(GL_VERTEX_ARRAY);
  SDL::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glEnableClientState(GL_COLOR_ARRAY);

  SDL::glVertexPointer(2, GL_FLOAT, 0, drawBuffers->positions.data());
  SDL::glTexCoordPointer(2, GL_FLOAT, 0, drawBuffers->texCoords.data());
  SDL::glColorPointer(4, GL_UNSIGNED_BYTE, 0, drawBuffers->colors.data());
  checkOpenglError();

  setBlendingMode(blendMode);
  for (auto& elem : drawBuffers->elements) {
    if (elem.blendMode != blendMode)
      continue;
    int texId = textureIds[elem.particleDefId];
    auto& tex = textures[texId];
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
