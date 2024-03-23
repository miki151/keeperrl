#pragma once

#include "fx_base.h"
#include "fx_texture_name.h"
#include "fx_rect.h"
#include "texture.h"

class Framebuffer;

namespace fx {

// FXRenderer can draw effects in two different ways: ordered & unordered
// - Ordered effects are rendered to framebuffers separately so that they can be
//   composed properly with game objects;
// - Unordered effects are drawn together into screen-sized framebuffer and are
//   drawn to screen together
//
// Effects also have layers which allow additional separation; back layer is
// always drawn as unordered;
class FXRenderer {
public:
  FXRenderer(DirectoryPath, FXManager &);
  ~FXRenderer();

  FXRenderer(const FXRenderer &) = delete;
  void operator=(const FXRenderer &) = delete;

  void setView(float zoomX, float zoomY, float offsetX, float offsetY, int w, int h);
  void prepareOrdered();

  // TODO: span<> would be very useful
  // TODO: pass proper ids
  void drawOrdered(const int* systemIds, int count, float offsetX, float offsetY, Color);

  void drawUnordered(Layer);

  bool useFramebuffer = true;
  pair<unsigned, unsigned> fboIds(bool ordered) const;
  IVec2 fboSize() const;
  void loadTextures();

  private:
  struct View {
    float zoomX;
    float zoomY;
    FVec2 offset;
    IVec2 size;
  };

  IRect visibleTiles(const View&);
  void drawParticles(const View&, BlendMode);
  void drawParticles(FVec2 offset, Framebuffer&, Framebuffer&);

  static IRect boundingBox(const DrawParticle*, int count);
  void printSystemDrawsInfo() const;
  IVec2 allocateFboSpace();

  FXManager& mgr;
  vector<Texture> textures;
  vector<FVec2> textureScales;
  EnumMap<TextureName, int> textureIds;

  struct SystemDrawInfo;
  vector<SystemDrawInfo> systemDraws;
  vector<DrawParticle> orderedParticles, tempParticles;
  vector<FRect> tempRects;

  void applyTexScale();

  View worldView;
  IRect fboView;
  unique_ptr<DrawBuffers> drawBuffers;
  unique_ptr<Framebuffer> orderedBlendFBO, orderedAddFBO;
  unique_ptr<Framebuffer> blendFBO, addFBO;
  DirectoryPath texturesPath;
};
}
