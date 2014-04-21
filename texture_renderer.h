#ifndef _TEXTURE_RENDERER_H
#define _TEXTURE_RENDERER_H

#include <SFML/Graphics.hpp>

#include "util.h"
#include "renderer.h"

using sf::Texture;
using sf::RenderTexture;

class TextureRenderer : public Renderer {
  public:
  void initialize(int width, int height);
  const Texture& getTexture();
  void clear();

  private:
  RenderTexture tex;
};

#endif
