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

RICH_ENUM(ColorId,
  WHITE,
  YELLOW,
  LIGHT_BROWN,
  ORANGE_BROWN,
  BROWN,
  DARK_BROWN,
  LIGHT_GRAY,
  GRAY,
  ALMOST_GRAY,
  DARK_GRAY,
  ALMOST_BLACK,
  ALMOST_DARK_GRAY,
  BLACK,
  ALMOST_WHITE,
  GREEN,
  LIGHT_GREEN,
  DARK_GREEN,
  RED,
  LIGHT_RED,
  PINK,
  ORANGE,
  BLUE,
  DARK_BLUE,
  NIGHT_BLUE,
  LIGHT_BLUE,
  PURPLE,
  VIOLET,
  TRANSLUCENT_BLACK,
  TRANSPARENT
);

Color transparency(const Color& color, int trans);

extern EnumMap<ColorId, Color> colors;

enum class SpriteId {
  BUILDINGS,
  MINIONS,
  LIBRARY,
  WORKSHOP,
  DIPLOMACY,
  HELP
};

class ViewObject;

class Renderer {
  public: 
  const static int textSize = 19;
  enum FontId { TEXT_FONT, TILE_FONT, SYMBOL_FONT };
  int getTextLength(string s);
  void drawText(FontId, int size, Color color, int x, int y, String s, bool center = false);
  void drawTextWithHotkey(Color color, int x, int y, const string& text, char key);
  void drawText(Color color, int x, int y, string s, bool center = false, int size = textSize);
  void drawText(Color color, int x, int y, const char* c, bool center = false, int size = textSize);
  void drawImage(int px, int py, const Image& image, double scale = 1);
  void drawImage(int px, int py, int kx, int ky, const Image& image, double scale = 1);
  void drawImage(Rectangle, const Image& image, double scale = 1);
  void drawSprite(Vec2 pos, Vec2 spos, Vec2 size, const Texture& t, Optional<Color> color = Nothing());
  void drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t, int dw = -1, int dh = -1,
      Optional<Color> color = Nothing());
  void drawSprite(int x, int y, SpriteId, Optional<Color> color = Nothing());
  void drawSprite(Vec2 pos, Vec2 stretchSize, const Texture&);
  void drawFilledRectangle(const Rectangle& t, Color color, Optional<Color> outline = Nothing());
  void drawFilledRectangle(int px, int py, int kx, int ky, Color color, Optional<Color> outline = Nothing());
  void drawViewObject(int x, int y, const ViewObject&, bool useSprite, double scale = 1);
  void drawViewObject(int x, int y, ViewId, bool useSprite, double scale = 1);
  static Color getBleedingColor(const ViewObject&);
  int getWidth();
  int getHeight();
  static bool loadTilesFromDir(const string& path, Vec2 size);
  static bool loadTilesFromFile(const string& path, Vec2 size);
  static void setNominalSize(Vec2);
  static String toUnicode(const string&);

  void initialize(int width, int height, string title);
  void drawAndClearBuffer();
  void resize(int width, int height);
  bool pollEvent(Event&, Event::EventType);
  bool pollEvent(Event&);
  void flushEvents(Event::EventType);
  void flushAllEvents();
  void waitEvent(Event&);
  Vec2 getMousePos();
  bool leftButtonPressed();
  bool rightButtonPressed();

  void startMonkey();
  bool isMonkey();
  Event getRandomEvent();

  struct TileCoords {
    Vec2 pos;
    int texNum;
  };

  static TileCoords getTileCoords(const string&);

  static vector<Texture> tiles;
  static vector<Vec2> tileSize;
  static Vec2 nominalSize;

  private:
  static map<string, TileCoords> tileCoords;
  bool pollEventWorkaroundMouseReleaseBug(Event&);
  RenderWindow* display = nullptr;
  sf::View* sfView;
  bool monkey = false;
  deque<Event> eventQueue;
  bool genReleaseEvent = false;
};

#endif
