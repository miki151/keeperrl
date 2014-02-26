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

namespace colors {
  extern const Color white;
  extern const Color yellow;
  extern const Color lightBrown;
  extern const Color orangeBrown;
  extern const Color brown;
  extern const Color darkBrown;
  extern const Color lightGray;
  extern const Color gray;
  extern const Color almostGray;
  extern const Color darkGray;
  extern const Color almostBlack;
  extern const Color almostDarkGray;
  extern const Color black;
  extern const Color almostWhite;
  extern const Color green;
  extern const Color lightGreen;
  extern const Color darkGreen;
  extern const Color red;
  extern const Color lightRed;
  extern const Color pink;
  extern const Color orange;
  extern const Color blue;
  extern const Color darkBlue;
  extern const Color lightBlue;
  extern const Color purple;
  extern const Color violet;
  extern const Color translucentBlack;

  Color transparency(const Color& color, int trans);
}

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

  static vector<Texture> tiles;
  const static vector<int> tileSize;
  const static int nominalSize;

  private:
  RenderWindow* display = nullptr;
  sf::View* sfView;
  stack<Vec2> translations;
  Vec2 translation;
};

#endif
