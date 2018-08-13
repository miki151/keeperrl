#pragma once

#include "fx_base.h"
#include "texture.h"

namespace fx {

class FXRenderer {
public:
  FXRenderer(DirectoryPath, FXManager &);
  ~FXRenderer();

  FXRenderer(const FXRenderer &) = delete;
  void operator=(const FXRenderer &) = delete;

  void draw(float zoom, float offsetX, float offsetY);

  // TODO: better way to communicate with FXRenderer ?
  static FXRenderer *getInstance();

  private:
  // TODO: remove m_
  FXManager& m_mgr;
  vector<Texture> m_textures;
  vector<FVec2> m_textureScales;
  // Maps particleDefId to textureId
  vector<int> m_textureIds;

  void applyTexScale();

  unique_ptr<DrawBuffers> m_drawBuffers;
};
}
