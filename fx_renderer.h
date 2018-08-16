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

  // TODO: remove m_
  FXManager& m_mgr;
  vector<Texture> m_textures;
  vector<FVec2> m_textureScales;
  // Maps particleDefId to textureId
  vector<int> m_textureIds;

  void applyTexScale();

  unique_ptr<DrawBuffers> m_drawBuffers;
  unique_ptr<Framebuffer> m_framebuffer;
};
}
