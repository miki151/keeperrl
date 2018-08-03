#pragma once

#include "fx_base.h"
#include "renderer.h"

namespace fx {

class FXRenderer {
public:
  FXRenderer(DirectoryPath, FXManager &);
  ~FXRenderer();

  FXRenderer(const FXRenderer &) = delete;
  void operator=(const FXRenderer &) = delete;

  void draw(float zoom, Vec2 offset);

  // TODO: better way to communicate with FXRenderer ?
  static FXRenderer *getInstance();

private:
  FXManager &m_mgr;
  vector<Texture> m_textures;
  vector<FVec2> m_textureScales;

  struct Element {
    int firstVertex;
    int numVertices;
    int textureId;
  };

  vector<FVec2> m_positions;
  vector<FVec2> m_texCoords;
  vector<unsigned int> m_colors;
  vector<Element> m_elements;
};
}
