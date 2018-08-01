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

#pragma once

#include "stdafx.h"
#include "sdl.h"
#include "util.h"
#include "file_path.h"
#include "animation_id.h"

struct Color : public SDL::SDL_Color {
  Color(Uint8, Uint8, Uint8, Uint8 = 255);
  Color transparency(int);
  static Color f(double, double, double, double = 1.0);
  Color operator* (Color);
  Color();
  void applyGl() const;

  static Color WHITE;
  static Color MAIN_MENU_ON;
  static Color MAIN_MENU_OFF;
  static Color YELLOW;
  static Color LIGHT_BROWN;
  static Color ORANGE_BROWN;
  static Color BROWN;
  static Color DARK_BROWN;
  static Color LIGHT_GRAY;
  static Color GRAY;
  static Color ALMOST_GRAY;
  static Color DARK_GRAY;
  static Color ALMOST_BLACK;
  static Color ALMOST_DARK_GRAY;
  static Color BLACK;
  static Color ALMOST_WHITE;
  static Color GREEN;
  static Color LIGHT_GREEN;
  static Color DARK_GREEN;
  static Color RED;
  static Color LIGHT_RED;
  static Color PINK;
  static Color ORANGE;
  static Color BLUE;
  static Color DARK_BLUE;
  static Color NIGHT_BLUE;
  static Color LIGHT_BLUE;
  static Color SKY_BLUE;
  static Color PURPLE;
  static Color VIOLET;
  static Color TRANSLUCENT_BLACK;
  static Color TRANSPARENT;
};

enum class SpriteId {
  BUILDINGS,
  MINIONS,
  LIBRARY,
  WORKSHOP,
  DIPLOMACY,
  HELP
};

class ViewObject;
class Clock;

struct sth_stash;

class Texture {
  public:
  Texture();
  Texture(const Texture&) = delete;
  Texture(Texture&&);
  Texture& operator = (Texture&&);
  Texture(const FilePath& path);
  Texture(const FilePath& path, int px, int py, int kx, int ky);
  explicit Texture(SDL::SDL_Surface*);
  static optional<Texture> loadMaybe(const FilePath&);

  optional<SDL::GLenum> loadFromMaybe(SDL::SDL_Surface*);
  const Vec2& getSize() const;

  ~Texture();

  private:
  friend class Renderer;
  void addTexCoord(int x, int y) const;
  optional<SDL::GLuint> texId;
  Vec2 size;
  Vec2 realSize;
  optional<FilePath> path;
};

class Renderer {
  public:
    static constexpr int nominalSize = 24;

  class TileCoord {
    public:
    TileCoord();
    TileCoord(const TileCoord& o) : pos(o.pos), texNum(o.texNum) {}

    private:
    friend class Renderer;
    TileCoord(Vec2, int);
    Vec2 pos;
    int texNum;
  };

  Renderer(Clock*, const string& windowTile, const DirectoryPath& fontPath, const FilePath& cursorPath,
      const FilePath& clickedCursorPath);
  void setFullscreen(bool);
  void setFullscreenMode(int);
  void setVsync(bool);
  void setZoom(int);
  int getZoom();
  void enableCustomCursor(bool);
  void initialize();
  bool isFullscreen();
  void showError(const string&);
  static vector<string> getFullscreenResolutions();
  const static int textSize = 19;
  const static int smallTextSize = 14;
  static SDL::SDL_Surface* createSurface(int w, int h);
  static SDL::SDL_Surface* createPowerOfTwoSurface(SDL::SDL_Surface*);
  enum FontId { TEXT_FONT, TILE_FONT, SYMBOL_FONT };
  int getTextLength(const string& s, int size = textSize, FontId = TEXT_FONT);
  Vec2 getTextSize(const string& s, int size = textSize, FontId = TEXT_FONT);
  enum CenterType { NONE, HOR, VER, HOR_VER };
  void drawText(FontId, int size, Color, Vec2 pos, const string&, CenterType center = NONE);
  void drawTextWithHotkey(Color, Vec2 pos, const string&, char key);
  void drawText(Color, Vec2 pos, const string&, CenterType center = NONE, int size = textSize);
  void drawText(Color, Vec2 pos, const char* c, CenterType center = NONE, int size = textSize);
  void drawImage(int px, int py, const Texture&, double scale = 1, optional<Color> = none);
  void drawImage(int px, int py, int kx, int ky, const Texture&, double scale = 1);
  void drawImage(Rectangle target, Rectangle source, const Texture&);
  struct SpriteOrientation {
    SpriteOrientation() : x(1), y(0), horizontalFlip(false) {}
    SpriteOrientation(Vec2 d, bool f) : horizontalFlip(f) { double l = d.lengthD(); x = d.x / l; y = d.y / l; }
    SpriteOrientation(bool vFlip, bool hFlip) : x(vFlip ? -1 : 1), y(0), horizontalFlip(hFlip ^ vFlip) {}
    double x;
    double y;
    bool horizontalFlip;
  };
  void drawSprite(Vec2 pos, Vec2 source, Vec2 size, const Texture&, optional<Vec2> targetSize = none,
      optional<Color> color = none,SpriteOrientation = {});
  void drawSprite(int x, int y, SpriteId, optional<Color> color = none);
  void drawSprite(Vec2 pos, Vec2 stretchSize, const Texture&);
  void drawFilledRectangle(const Rectangle&, Color, optional<Color> outline = none);
  void drawFilledRectangle(int px, int py, int kx, int ky, Color color, optional<Color> outline = none);
  void drawPoint(Vec2 pos, Color color, int size);
  void drawViewObject(Vec2 pos, const ViewObject&, bool useSprite, Vec2 size);
  void drawViewObject(Vec2 pos, const ViewObject&, bool useSprite, double scale = 1, Color = Color::WHITE);
  void drawViewObject(Vec2 pos, const ViewObject&);
  void drawViewObject(Vec2 pos, ViewId, bool useSprite, double scale = 1, Color = Color::WHITE);
  void drawViewObject(Vec2 pos, ViewId, bool useSprite, Vec2 size, Color = Color::WHITE, SpriteOrientation = {});
  void drawViewObject(Vec2 pos, ViewId, Color = Color::WHITE);
  void drawAsciiBackground(ViewId, Rectangle bounds);
  void drawTile(Vec2 pos, const vector<TileCoord>&, double scale = 1, Color = Color::WHITE);
  void drawTile(Vec2 pos, const vector<TileCoord>&, Vec2 size, Color = Color::WHITE, SpriteOrientation orientation = {});
  void setScissor(optional<Rectangle>);
  void addQuad(const Rectangle&, Color);
  void drawAnimation(AnimationId, Vec2, double state, Vec2 squareSize, Dir orientation);
  Vec2 getSize();

  void drawAndClearBuffer();
  void resize(int width, int height);
  bool pollEvent(Event&, EventType);
  bool pollEvent(Event&);
  void flushEvents(EventType);
  void waitEvent(Event&);
  Vec2 getMousePos();

  void setTopLayer();
  void popLayer();

  void startMonkey();
  bool isMonkey();

  void printSystemInfo(ostream&);

  const vector<TileCoord>& getTileCoord(const string&);
  vector<Texture> tiles;

  static void putPixel(SDL::SDL_Surface*, Vec2, Color);
  void addTilesDirectory(const DirectoryPath&, Vec2 size);
  void setAnimationsDirectory(const DirectoryPath&);
  void loadTiles();
  void makeScreenshot(const FilePath&);

  private:
  friend class Texture;
  optional<Texture> textTexture;
  Renderer(const Renderer&);
  map<string, vector<TileCoord>> tileCoords;
  struct AnimationInfo {
    Texture tex;
    int numFrames;
  };
  EnumMap<AnimationId, optional<AnimationInfo>> animations;
  bool pollEventOrFromQueue(Event&);
  void considerMouseMoveEvent(Event&);
  void considerMouseCursorAnim(Event&);
  void zoomMousePos(Event&);
  void updateResolution();
  void initOpenGL();
  SDL::SDL_Window* window;
  int width, height;
  bool monkey = false;
  deque<Event> eventQueue;
  bool genReleaseEvent = false;
  Vec2 mousePos;
  struct FontSet {
    int textFont;
    int symbolFont;
  };
  FontSet fonts;
  sth_stash* fontStash;
  void loadFonts(const DirectoryPath& fontPath, FontSet&);
  int getFont(Renderer::FontId);
  optional<thread::id> renderThreadId;
  bool fullscreen;
  int fullscreenMode;
  int zoom = 1;
  bool cursorEnabled = true;
  void reloadCursors();
  FilePath cursorPath;
  FilePath clickedCursorPath;
  SDL::SDL_Cursor* originalCursor;
  SDL::SDL_Cursor* cursor;
  SDL::SDL_Cursor* cursorClicked;
  SDL::SDL_Surface* loadScaledSurface(const FilePath& path, double scale);
  optional<SDL::GLuint> currentTexture;
  void drawSprite(const Texture& t, Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 p, Vec2 k, optional<Color> color);
  void drawSprite(const Texture& t, Vec2 topLeft, Vec2 bottomRight, Vec2 p, Vec2 k, optional<Color> color);
  struct DeferredSprite {
    Vec2 a, b, c, d;
    Vec2 p, k;
    Vec2 realSize;
    optional<Color> color;
  };
  vector<DeferredSprite> deferredSprites;
  void renderDeferredSprites();
  vector<Rectangle> scissorStack;
  void loadTilesFromDir(const DirectoryPath&, Vec2 size, int setWidth);
  struct TileDirectory {
    DirectoryPath path;
    Vec2 size;
  };
  vector<TileDirectory> tileDirectories;
  optional<DirectoryPath> animationDirectory;
  Clock* clock;
};

