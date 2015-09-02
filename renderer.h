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

  Renderer(const string& windowTile, Vec2 nominalTileSize, const string& fontPath);
  void setFullscreen(bool);
  void setFullscreenMode(int);
  void setZoom(int);
  void initialize();
  bool isFullscreen();
  static vector<string> getFullscreenResolutions();
  const static int textSize = 19;
  enum FontId { TEXT_FONT, TILE_FONT, SYMBOL_FONT };
  int getTextLength(string s);
  enum CenterType { NONE, HOR, VER, HOR_VER };
  void drawText(FontId, int size, Color, int x, int y, String, CenterType center = NONE);
  void drawTextWithHotkey(Color, int x, int y, const string&, char key);
  void drawText(Color, int x, int y, string, CenterType center = NONE, int size = textSize);
  void drawText(Color, int x, int y, const char* c, CenterType center = NONE, int size = textSize);
  void drawImage(int px, int py, const Texture&, double scale = 1);
  void drawImage(int px, int py, int kx, int ky, const Texture&, double scale = 1);
  void drawImage(Rectangle target, Rectangle source, const Texture&);
  void drawSprite(Vec2 pos, Vec2 spos, Vec2 size, const Texture&, optional<Color> color,
      optional<Vec2> stretchSize = none);
  void drawSprite(Vec2 pos, Vec2 source, Vec2 size, const Texture&, Vec2 targetSize = Vec2(-1, -1),
      optional<Color> color = none);
  void drawSprite(int x, int y, SpriteId, optional<Color> color = none);
  void drawSprite(Vec2 pos, Vec2 stretchSize, const Texture&);
  void drawFilledRectangle(const Rectangle&, Color, optional<Color> outline = none);
  void drawFilledRectangle(int px, int py, int kx, int ky, Color color, optional<Color> outline = none);
  void drawViewObject(Vec2 pos, const ViewObject&, bool useSprite, Vec2 size);
  void drawViewObject(Vec2 pos, const ViewObject&, bool useSprite, double scale = 1);
  void drawViewObject(Vec2 pos, ViewId, bool useSprite, double scale = 1, Color = colors[ColorId::WHITE]);
  void drawViewObject(Vec2 pos, ViewId, bool useSprite, Vec2 size, Color = colors[ColorId::WHITE]);
  void drawTile(Vec2 pos, TileCoord coord, double scale = 1, Color = colors[ColorId::WHITE]);
  void drawTile(Vec2 pos, TileCoord coord, Vec2 size, Color = colors[ColorId::WHITE], bool hFlip = false,
      bool vFlip = false);
  void addQuad(const Rectangle&, Color);
  void drawQuads();
  static Color getBleedingColor(const ViewObject&);
  Vec2 getSize();
  bool loadTilesFromDir(const string& path, Vec2 size);
  bool loadTilesFromFile(const string& path, Vec2 size);
  static String toUnicode(const string&);

  void drawAndClearBuffer();
  void resize(int width, int height);
  bool pollEvent(Event&, Event::EventType);
  bool pollEvent(Event&);
  void flushEvents(Event::EventType);
  void waitEvent(Event&);
  Vec2 getMousePos();

  void setTopLayer();
  void popLayer();

  void startMonkey();
  bool isMonkey();

  void printSystemInfo(ostream&);

  TileCoord getTileCoord(const string&);
  Vec2 getNominalSize() const;
  vector<Texture> tiles;

  private:
  Renderer(const Renderer&);
  vector<Vec2> tileSize;
  Vec2 nominalSize;
  map<string, TileCoord> tileCoords;
  bool pollEventWorkaroundMouseReleaseBug(Event&);
  bool pollEventOrFromQueue(Event&);
  void considerMouseMoveEvent(Event&);
  void zoomMousePos(Event&);
  void updateResolution();
  Event getRandomEvent();
  RenderWindow display;
  bool monkey = false;
  deque<Event> eventQueue;
  bool genReleaseEvent = false;
  void addRenderElem(function<void()>);
  stack<int> layerStack;
  int currentLayer = 0;
  array<vector<function<void()>>, 2> renderList;
  vector<Vertex> quads;
  Vec2 mousePos;
  Font textFont;
  Font tileFont;
  Font symbolFont;
  Font& getFont(Renderer::FontId);
  optional<thread::id> renderThreadId;
  bool fullscreen;
  int fullscreenMode;
  int zoom = 1;
};

#endif
