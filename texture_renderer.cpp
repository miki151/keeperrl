#include "stdafx.h"
#include "texture_renderer.h"

void TextureRenderer::initialize(int width, int height) {
  tex.create(width, height);
  Renderer::initialize(&tex, width, height);
}

const Texture& TextureRenderer::getTexture() {
  tex.display();
  return tex.getTexture();
}

void TextureRenderer::clear() {
  tex.clear(Color(0, 0, 0, 0));
}
