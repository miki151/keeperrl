#pragma once

#include "fx_base.h"
#include "fx_texture_name.h"
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
  pair<unsigned, unsigned> fboIds() const;
  IVec2 fboSize() const;

  private:
  struct View;
  IRect visibleTiles(const View&);

  void initFramebuffer(IVec2);
  void drawParticles(const View&, BlendMode);

  FXManager& mgr;
  vector<Texture> textures;
  vector<FVec2> textureScales;
  EnumMap<TextureName, int> textureIds;

  void applyTexScale();

  unique_ptr<DrawBuffers> drawBuffers;
  unique_ptr<Framebuffer> blendFBO, addFBO;
};
}
