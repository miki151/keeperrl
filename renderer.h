/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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
using sf::RenderTarget;
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
  extern const Color nightBlue;
  extern const Color lightBlue;
  extern const Color purple;
  extern const Color violet;
  extern const Color translucentBlack;

  Color transparency(const Color& color, int trans);
}

enum class SpriteId {
  BUILDINGS,
  MINIONS,
  LIBRARY,
  WORKSHOP,
  DIPLOMACY,
  HELP,
};

class Renderer {
  public: 
  const static int textSize = 19;
  enum FontId { TEXT_FONT, TILE_FONT, SYMBOL_FONT };
  void initialize(RenderTarget*, int width, int height);
  int getTextLength(string s);
  void drawText(FontId, int size, Color color, int x, int y, String s, bool center = false);
  void drawTextWithHotkey(Color color, int x, int y, const string& text, char key);
  void drawText(Color color, int x, int y, string s, bool center = false, int size = textSize);
  void drawText(Color color, int x, int y, const char* c, bool center = false, int size = textSize);
  void drawImage(int px, int py, const Image& image, double scale = 1);
  void drawImage(int px, int py, int kx, int ky, const Image& image, double scale = 1);
  void drawSprite(Vec2 pos, Vec2 spos, Vec2 size, const Texture& t, Optional<Color> color = Nothing());
  void drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t, int dw = -1, int dh = -1,
      Optional<Color> color = Nothing());
  void drawSprite(int x, int y, SpriteId, Optional<Color> color = Nothing());
  void drawFilledRectangle(const Rectangle& t, Color color, Optional<Color> outline = Nothing());
  void drawFilledRectangle(int px, int py, int kx, int ky, Color color, Optional<Color> outline = Nothing());
  int getWidth();
  int getHeight();

  static vector<Texture> tiles;
  const static vector<int> tileSize;
  const static int nominalSize;

  private:
  RenderTarget* display = nullptr;
};

#endif
