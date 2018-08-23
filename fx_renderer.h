#pragma once

#include "fx_base.h"
#include "texture.h"

class Framebuffer;

namespace fx {

class FXRenderer {
public:
  FXRenderer(DirectoryPath, FXManager &);
  ~FXRenderer();

  FXRenderer(const FXRenderer &) = delete;
  void operator=(const FXRenderer &) = delete;

  void draw(float zoom, float offsetX, float offsetY, int w, int h);

  // TODO: better way to communicate with FXRenderer ?
  static FXRenderer *getInstance();

  bool useFramebuffer = true;

  private:
  struct View;
  IRect visibleTiles(const View&);

  void initFramebuffer(IVec2);
  void drawParticles(const View&, BlendMode);
  void setBlendingMode(BlendMode);

  FXManager& mgr;
  vector<Texture> textures;
  vector<FVec2> textureScales;
  // Maps particleDefId to textureId
  vector<int> textureIds;

  void applyTexScale();

  unique_ptr<DrawBuffers> drawBuffers;
  unique_ptr<Framebuffer> framebuffer;
};
}
