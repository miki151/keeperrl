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

void FXRenderer::initFramebuffer(IVec2 size) {
  if (!Framebuffer::isExtensionAvailable())
    return;
  if (!blendFBO || blendFBO->width != size.x || blendFBO->height != size.y) {
    INFO << "FX: creating FBO (" << size.x << ", " << size.y << ")";
    blendFBO = std::make_unique<Framebuffer>(size.x, size.y);
    addFBO = std::make_unique<Framebuffer>(size.x, size.y);
  }
}

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

void FXRenderer::draw(float zoom, float offsetX, float offsetY, int w, int h) {
  CHECK_OPENGL_ERROR();
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

  if (blendFBO && addFBO && useFramebuffer) {
    pushOpenglView();
    FVec2 fboOffset = -FVec2(fboView.min() * nominalSize);

    blendFBO->bind();
    SDL::glPushAttrib(GL_ENABLE_BIT);
    SDL::glDisable(GL_SCISSOR_TEST);
    setupOpenglView(blendFBO->width, blendFBO->height, 1.0f);

    SDL::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    SDL::glClear(GL_COLOR_BUFFER_BIT);
    SDL::glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    drawParticles({1.0f, fboOffset, fboScreenSize}, BlendMode::normal);

    addFBO->bind();
    SDL::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    SDL::glClear(GL_COLOR_BUFFER_BIT);

    /* // Mode toggler; useful for testing
	static bool mode0 = false;
	static double time = fwk::getTime();
	if(fwk::getTime() - time > 4.0) {
		time = fwk::getTime();
		mode0 ^= 1;
	}*/

    // TODO: Each effect could control how alpha builds up
    SDL::glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    drawParticles({1.0f, fboOffset, fboScreenSize}, BlendMode::additive);

    Framebuffer::unbind();
    SDL::glPopAttrib();
    popOpenglView();

    glColor(Color::WHITE);

    int defaultMode = 0, defaultCombine = 0;
    SDL::glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &defaultMode);
    SDL::glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB, &defaultCombine);

    // TODO: positioning is wrong for non-integral zoom values
    FVec2 c1 = FVec2(fboView.min() * nominalSize * zoom) + view.offset;
    FVec2 c2 = FVec2(fboView.max() * nominalSize * zoom) + view.offset;

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
    drawParticles(view, BlendMode::normal);
    SDL::glBlendFunc(GL_ONE, GL_ONE);
    drawParticles(view, BlendMode::additive);
  }

  SDL::glPopAttrib();
  SDL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  CHECK_OPENGL_ERROR();
}

pair<unsigned, unsigned> FXRenderer::fboIds() const {
  if (blendFBO && addFBO)
    return make_pair(blendFBO->texId, addFBO->texId);
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
