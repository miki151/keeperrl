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
using sf::Vertex;
using sf::Event;

RICH_ENUM(ColorId,
  WHITE,
  MAIN_MENU_ON,
  MAIN_MENU_OFF,
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
  class TileCoord {
    public:
    TileCoord(Vec2, int);
    TileCoord();
    TileCoord(const TileCoord& o) : pos(o.pos), texNum(o.texNum) {}

    Vec2 pos;
    int texNum;
  };

  Renderer(const string& windowTile, Vec2 nominalTileSize);
  void initialize();
  const static int textSize = 19;
  enum FontId { TEXT_FONT, TILE_FONT, SYMBOL_FONT };
  int getTextLength(string s);
  void drawText(FontId, int size, Color, int x, int y, String, bool center = false);
  void drawTextWithHotkey(Color, int x, int y, const string&, char key);
  void drawText(Color, int x, int y, string, bool center = false, int size = textSize);
  void drawText(Color, int x, int y, const char* c, bool center = false, int size = textSize);
  void drawImage(int px, int py, const Texture&, double scale = 1);
  void drawImage(int px, int py, int kx, int ky, const Texture&, double scale = 1);
  void drawImage(Rectangle target, Rectangle source, const Texture&);
  void drawSprite(Vec2 pos, Vec2 spos, Vec2 size, const Texture&, optional<Color> color = none,
      optional<Vec2> stretchSize = none);
  void drawSprite(int x, int y, int px, int py, int w, int h, const Texture&, int dw = -1, int dh = -1,
      optional<Color> color = none);
  void drawSprite(int x, int y, SpriteId, optional<Color> color = none);
  void drawSprite(Vec2 pos, Vec2 stretchSize, const Texture&);
  void drawFilledRectangle(const Rectangle&, Color, optional<Color> outline = none);
  void drawFilledRectangle(int px, int py, int kx, int ky, Color color, optional<Color> outline = none);
  void drawViewObject(int x, int y, const ViewObject&, bool useSprite, double scale = 1);
  void drawViewObject(int x, int y, ViewId, bool useSprite, double scale = 1, Color = colors[ColorId::WHITE]);
  void drawTile(int x, int y, TileCoord coord, double scale = 1, Color = colors[ColorId::WHITE]);
  void drawTile(int x, int y, TileCoord coord, int sizeX, int sizeY, Color = colors[ColorId::WHITE]);
  void addQuad(const Rectangle&, Color);
  void drawQuads();
  static Color getBleedingColor(const ViewObject&);
  int getWidth();
  int getHeight();
  bool loadTilesFromDir(const string& path, Vec2 size);
  bool loadTilesFromFile(const string& path, Vec2 size);
  static String toUnicode(const string&);

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

  TileCoord getTileCoord(const string&);
  Vec2 getNominalSize() const;
  vector<Texture> tiles;

  private:
  vector<Vec2> tileSize;
  Vec2 nominalSize;
  map<string, TileCoord> tileCoords;
  bool pollEventWorkaroundMouseReleaseBug(Event&);
  bool pollEventOrFromQueue(Event&);
  RenderWindow* display = nullptr;
  sf::View* sfView;
  bool monkey = false;
  deque<Event> eventQueue;
  bool genReleaseEvent = false;
  vector<function<void()>> renderList;
  vector<Vertex> quads;
};

#endif
