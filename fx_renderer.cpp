#include "fx_renderer.h"

#include "fx_manager.h"
#include "fx_defs.h"
#include "fx_particle_system.h"
#include "fx_draw_buffers.h"

#include "opengl.h"
#include "framebuffer.h"
#include "renderer.h"

namespace fx {

static constexpr int nominalSize = Renderer::nominalSize;

struct FXRenderer::SystemDrawInfo {
  bool empty() const {
    return numParticles == 0;
  }

  IRect worldRect; // in pixels
  IVec2 fboPos;
  int firstParticle = 0, numParticles = 0;
};

static FXRenderer *s_instance = nullptr;

FXRenderer *FXRenderer::getInstance() { return s_instance; }

FXRenderer::FXRenderer(DirectoryPath dataPath, FXManager& mgr) : mgr(mgr) {
  textures.reserve(EnumInfo<TextureName>::size);
  textureScales.reserve(textures.size());
  drawBuffers = std::make_unique<DrawBuffers>();

  for (auto texName : ENUM_ALL(TextureName)) {
    auto& tdef = mgr[texName];
    auto path = dataPath.file(tdef.fileName);
    int id = -1;
    for (int n = 0; n < (int)textures.size(); n++)
      if (textures[n].getPath() == path) {
        id = n;
        break;
      }

    if (id == -1) {
      id = textures.size();
      textures.emplace_back(path);
      textures.back().setParams(Texture::Filter::linear, Texture::Wrapping::clamp);
      auto tsize = textures.back().getSize(), rsize = textures.back().getRealSize();
      FVec2 scale(float(tsize.x) / float(rsize.x), float(tsize.y) / float(rsize.y));
      textureScales.emplace_back(scale);
    }
    textureIds[texName] = id;
  }
  CHECK(s_instance == nullptr && "There can be only one!");
  s_instance = this;
}

FXRenderer::~FXRenderer() { s_instance = nullptr; }

void FXRenderer::applyTexScale() {
  auto& elements = drawBuffers->elements;
  auto& texCoords = drawBuffers->texCoords;

  for (auto& elem : elements) {
    auto scale = textureScales[textureIds[elem.texName]];
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

IRect FXRenderer::boundingBox(const DrawParticle* particles, int count) {
  if (count == 0)
    return IRect();

  FVec2 min = particles[0].positions[0];
  FVec2 max = min;

  // TODO: what to do with invalid data ?
  for (int n = 0; n < count; n++) {
    auto& particle = particles[n];
    for (auto& pt : particle.positions) {
      min = vmin(min, pt);
      max = vmax(max, pt);
    }
  }

  return {IVec2(min) - IVec2(1), IVec2(max) + IVec2(2)};
}

IVec2 FXRenderer::allocateFboSpace() {
  IVec2 size = blendFBO ? IVec2(orderedBlendFBO->width, orderedBlendFBO->height) : IVec2(256, 256);

  // TODO: BIG PROBLEM: what about effects which are diagonal? They will take a lot more
  // space than necessary; Maybe these effects should be drawn just like before
  vector<pair<int, int>> ids;
  ids.reserve(systemDraws.size());

  int totalPixels = 0;

  bool orderByHeight = false;
  for (int n = 0; n < systemDraws.size(); n++) {
    auto& draw = systemDraws[n];
    if (!draw.empty()) {
      int w = draw.worldRect.width(), h = draw.worldRect.height();
      while (size.x < w)
        size.x *= 2;
      while (size.y < h)
        size.y *= 2;
      ids.emplace_back(orderByHeight ? -h : 0, n);
      totalPixels += w * h;
    }
  }

  std::sort(begin(ids), end(ids));

  bool doesntFit = true;
  while (doesntFit) {
    IVec2 pos;
    int maxHeight = 0;

    doesntFit = false;
    for (auto idPair : ids) {
      int id = idPair.second;
      auto& draw = systemDraws[id];
      int w = draw.worldRect.width(), h = draw.worldRect.height();

      if (pos.x + w > size.x)
        pos = {0, pos.y + maxHeight};
      if (pos.y + h > size.y) {
        size.y *= 2;
        doesntFit = true;
        break;
      }
      draw.fboPos = pos;
      pos.x += w;
      maxHeight = max(maxHeight, h);
    }
  }

  // TODO: properly handle such situation
  DASSERT(size.x <= 2048 && size.y <= 2048);

  return size;
}

void FXRenderer::prepareOrdered(optional<Layer> layer) {
  auto ids = mgr.aliveSystems(); // TODO: alive ordered systems
  int maxId = 0;
  for (auto id : ids)
    maxId = max(maxId, (int)id);
  systemDraws.clear();
  systemDraws.resize(maxId + 1);
  tempParticles.clear();

  for (auto id : ids) {
    if (!mgr.get(id).orderedDraw)
      continue;

    // TODO: properly handle layers
    int first = (int)tempParticles.size();
    if (!layer || layer == Layer::back)
      mgr.genQuads(tempParticles, id, Layer::back);
    if (!layer || layer == Layer::front)
      mgr.genQuads(tempParticles, id, Layer::front);
    int count = (int)tempParticles.size() - first;

    if (count > 0) {
      auto rect = boundingBox(&tempParticles[first], count);
      systemDraws[id] = {rect, IVec2(), first, count};
    }
  }

  // TODO: obsługiwanie sytuacji bez FBOsów ?
  //if (!Framebuffer::isExtensionAvailable())
  auto fboSize = allocateFboSpace();
  if (!orderedBlendFBO || orderedBlendFBO->width != fboSize.x || orderedBlendFBO->height != fboSize.y) {
    INFO << "FX: creating FBO for ordered rendering (" << fboSize.x << ", " << fboSize.y << ")";
    orderedBlendFBO = std::make_unique<Framebuffer>(fboSize.x, fboSize.y);
    orderedAddFBO = std::make_unique<Framebuffer>(fboSize.x, fboSize.y);
  }

  // Positioning particles for FBO
  for (int n = 0; n < (int)systemDraws.size(); n++) {
    auto& draw = systemDraws[n];
    if (draw.empty())
      continue;
    FVec2 offset(draw.fboPos - draw.worldRect.min());

    for (int p = 0; p < draw.numParticles; p++) {
      auto& particle = tempParticles[draw.firstParticle + p];
      for (auto& pos : particle.positions)
        pos += offset;
    }
  }

  drawBuffers->clear();
  drawBuffers->add(tempParticles);
  drawParticles(FVec2(), *orderedBlendFBO, *orderedAddFBO);
}

void FXRenderer::printSystemDrawsInfo() const {
  for (int n = 0; n < (int)systemDraws.size(); n++) {
    auto& sys = systemDraws[n];
    if (sys.empty())
      continue;

    INFO << "FX System #" << n << ": " << " rect:(" << sys.worldRect.x() << ", "
         << sys.worldRect.y() << ") - (" << sys.worldRect.ex() << ", "
         << sys.worldRect.ey() << ")";
  }
}

void FXRenderer::setView(float zoom, float offsetX, float offsetY, int w, int h) {
  worldView = View{zoom, {offsetX, offsetY}, {w, h}};
  fboView = visibleTiles(worldView);
  auto size = fboView.size() * nominalSize;

  if (useFramebuffer) {
    DASSERT(Framebuffer::isExtensionAvailable());
    if (!blendFBO || blendFBO->width != size.x || blendFBO->height != size.y) {
      INFO << "FX: creating FBO (" << size.x << ", " << size.y << ")";
      blendFBO = std::make_unique<Framebuffer>(size.x, size.y);
      addFBO = std::make_unique<Framebuffer>(size.x, size.y);
    }
  }
}

void FXRenderer::drawParticles(FVec2 viewOffset, Framebuffer& blendFBO, Framebuffer& addFBO) {
  IVec2 viewSize(blendFBO.width, blendFBO.height);
  pushOpenglView();

  blendFBO.bind();
  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glDisable(GL_SCISSOR_TEST);
  setupOpenglView(blendFBO.width, blendFBO.height, 1.0f);

  SDL::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  SDL::glClear(GL_COLOR_BUFFER_BIT);
  SDL::glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

  drawParticles({1.0f, viewOffset, viewSize}, BlendMode::normal);

  addFBO.bind();
  SDL::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  SDL::glClear(GL_COLOR_BUFFER_BIT);

  // TODO: Each effect could control how alpha builds up
  SDL::glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  drawParticles({1.0f, viewOffset, viewSize}, BlendMode::additive);

  Framebuffer::unbind();
  SDL::glPopAttrib();
  popOpenglView();
}

void FXRenderer::drawOrdered(int systemIdx) {
  auto& draw = systemDraws[systemIdx];
  if (draw.empty())
    return;

  FRect rect(draw.worldRect);
  rect = rect * worldView.zoom + worldView.offset;
  FVec2 fboSize(orderedBlendFBO->width, orderedBlendFBO->height);
  auto invSize = vinv(fboSize);
  auto trect = FRect(IRect(draw.fboPos, draw.fboPos + draw.worldRect.size())) * invSize;

  // TODO: rendering performance
  auto drawQuad = [&]() {
    SDL::glBegin(GL_QUADS);
    SDL::glTexCoord2f(trect.x(), 1.0f - trect.ey()), SDL::glVertex2f(rect.x(), rect.ey());
    SDL::glTexCoord2f(trect.ex(), 1.0f - trect.ey()), SDL::glVertex2f(rect.ex(), rect.ey());
    SDL::glTexCoord2f(trect.ex(), 1.0f - trect.y()), SDL::glVertex2f(rect.ex(), rect.y());
    SDL::glTexCoord2f(trect.x(), 1.0f - trect.y()), SDL::glVertex2f(rect.x(), rect.y());
    SDL::glEnd();
  };

  int defaultMode = 0, defaultCombine = 0;
  SDL::glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &defaultMode);
  SDL::glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB, &defaultCombine);
  SDL::glEnable(GL_TEXTURE_2D);

  SDL::glBlendFunc(GL_ONE, GL_SRC_ALPHA);
  SDL::glBindTexture(GL_TEXTURE_2D, orderedBlendFBO->texId);
  drawQuad();

  // Here we're performing blend-add:
  // - for high alpha values we're blending
  // - for low alpha values we're adding
  // For this to work nicely, additive textures need properly prepared alpha channel
  SDL::glBindTexture(GL_TEXTURE_2D, orderedAddFBO->texId);
  SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // These states multiply alpha by itself
  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_TEXTURE);
  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
  drawQuad();

  // Here we should really multiply by (1 - a), not (1 - a^2), but it looks better
  SDL::glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
  drawQuad();

  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, defaultMode);
  SDL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, defaultCombine);
}

void FXRenderer::drawAllOrdered() {
  for (int n = 0; n < systemDraws.size(); n++)
    if (!systemDraws[n].empty())
      drawOrdered(n);
}

void FXRenderer::drawUnordered(optional<Layer> layer) {
  tempParticles.clear();
  drawBuffers->clear();
  mgr.genQuadsUnordered(tempParticles, layer);
  drawBuffers->add(tempParticles);
  if (drawBuffers->empty())
    return;

  CHECK_OPENGL_ERROR();

  applyTexScale();

  SDL::glPushAttrib(GL_ENABLE_BIT);
  SDL::glDisable(GL_DEPTH_TEST);
  SDL::glDisable(GL_CULL_FACE);
  SDL::glEnable(GL_TEXTURE_2D);

  if (blendFBO && addFBO && useFramebuffer) {
    drawParticles(-FVec2(fboView.min() * nominalSize), *blendFBO, *addFBO);
    glColor(Color::WHITE);

    int defaultMode = 0, defaultCombine = 0;
    SDL::glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &defaultMode);
    SDL::glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB, &defaultCombine);

    // TODO: positioning is wrong for non-integral zoom values
    FVec2 c1 = FVec2(fboView.min() * nominalSize * worldView.zoom) + worldView.offset;
    FVec2 c2 = FVec2(fboView.max() * nominalSize * worldView.zoom) + worldView.offset;

    SDL::glBlendFunc(GL_ONE, GL_SRC_ALPHA);
    SDL::glBindTexture(GL_TEXTURE_2D, blendFBO->texId);
    glQuad(c1.x, c1.y, c2.x, c2.y);

    // Here we're performing blend-add:
    // - for high alpha values we're blending
    // - for low alpha values we're adding
    // For this to work nicely, additive textures need properly prepared alpha channel
    SDL::glBindTexture(GL_TEXTURE_2D, addFBO->texId);
    SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // These states multiply alpha by itself
    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_TEXTURE);
    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
    glQuad(c1.x, c1.y, c2.x, c2.y);

    // Here we should really multiply by (1 - a), not (1 - a^2), but it looks better
    SDL::glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
    glQuad(c1.x, c1.y, c2.x, c2.y);

    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, defaultMode);
    SDL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, defaultCombine);
  } else {
    SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawParticles(worldView, BlendMode::normal);
    SDL::glBlendFunc(GL_ONE, GL_ONE);
    drawParticles(worldView, BlendMode::additive);
  }

  SDL::glPopAttrib();
  SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  CHECK_OPENGL_ERROR();
}

pair<unsigned, unsigned> FXRenderer::fboIds(bool ordered) const {
  if (!ordered && blendFBO && addFBO)
    return make_pair(blendFBO->texId, addFBO->texId);
  if (ordered && orderedBlendFBO && orderedAddFBO)
    return make_pair(orderedBlendFBO->texId, orderedAddFBO->texId);
  return make_pair(0u, 0u);
}

IVec2 FXRenderer::fboSize() const {
  return blendFBO ? IVec2(blendFBO->width, blendFBO->height) : IVec2();
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

  for (auto& elem : drawBuffers->elements) {
    auto& tdef = mgr[elem.texName];
    if (tdef.blendMode != blendMode)
      continue;
    auto& tex = textures[textureIds[elem.texName]];
    SDL::glBindTexture(GL_TEXTURE_2D, *tex.getTexId());
    SDL::glDrawArrays(GL_QUADS, elem.firstVertex, elem.numVertices);
  }

  SDL::glDisableClientState(GL_VERTEX_ARRAY);
  SDL::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  SDL::glDisableClientState(GL_COLOR_ARRAY);
  SDL::glPopAttrib();
  SDL::glPopMatrix();
}
}
