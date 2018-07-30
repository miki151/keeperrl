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

  struct Element {
    int first_vertex;
    int num_vertices;
    int texture_id;
  };

  vector<FVec2> m_positions;
  vector<FVec2> m_tex_coords;
  vector<unsigned int> m_colors;
  vector<Element> m_elements;
};
}
