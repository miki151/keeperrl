#ifndef _RENDERER_H
#define _RENDERER_H

#include <SFML/Graphics.hpp>

#include "util.h"

using sf::Font;
using sf::Color;
using sf::String;
using sf::Image;
using sf::Sprite;
using sf::Texture;
using sf::RenderWindow;
using sf::Event;

class Renderer {
  public: 
  const static int textSize = 19;
  enum FontId { TEXT_FONT, TILE_FONT, SYMBOL_FONT };
  void initialize(int width, int height, int color, string title);
  int getTextLength(string s);
  void drawText(FontId, int size, Color color, int x, int y, String s, bool center = false);
  void drawText(Color color, int x, int y, string s, bool center = false, int size = textSize);
  void drawText(Color color, int x, int y, const char* c, bool center = false, int size = textSize);
  void drawImage(int px, int py, const Image& image, double scale = 1);
  void drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t, int dw = -1, int dh = -1,
      Optional<Color> color = Nothing());
  void drawFilledRectangle(const Rectangle& t, Color color, Optional<Color> outline = Nothing());
  void drawFilledRectangle(int px, int py, int kx, int ky, Color color, Optional<Color> outline = Nothing());
  void drawAndClearBuffer();
  void resize(int width, int height);
  int getWidth();
  int getHeight();
  bool pollEvent(Event&);
  void waitEvent(Event&);
  Vec2 getMousePos();

  private:
  RenderWindow* display = nullptr;
  sf::View* sfView;
};

#endif
