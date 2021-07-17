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
#include "color.h"
#include "texture.h"
#include "font_id.h"

class ViewObject;
class Clock;
class TileSet;
struct TileCoord;

struct sth_stash;

class Renderer {
  public:
  static constexpr int nominalSize = 24;

  Renderer(Clock*, const string& windowTile, const DirectoryPath& fontPath, const FilePath& cursorPath,
      const FilePath& clickedCursorPath, const FilePath& logoPath);
  void setFullscreen(bool);
  void setFullscreenMode(int);
  void setVsync(bool);
  void setZoom(int);
  int getZoom();
  void enableCustomCursor(bool);
  void initialize();
  bool isFullscreen();
  void showError(const string&);
  TileSet& getTileSet();
  void setTileSet(TileSet*);
  static int textSize() { return 19; }
  static int smallTextSize() { return 14; }
  int getTextLength(const string& s, int size = textSize(), FontId = FontId::TEXT_FONT);
  Vec2 getTextSize(const string& s, int size = textSize(), FontId = FontId::TEXT_FONT);
  enum CenterType { NONE, HOR, VER, HOR_VER };
  void drawText(FontId, int size, Color, Vec2 pos, const string&, CenterType center = NONE);
  void drawTextWithHotkey(Color, Vec2 pos, const string&, char key);
  void drawText(Color, Vec2 pos, const string&, CenterType center = NONE, int size = textSize());
  void drawText(Color, Vec2 pos, const char* c, CenterType center = NONE, int size = textSize());
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
  void drawSprite(Vec2 pos, Vec2 stretchSize, const Texture&);
  void drawFilledRectangle(const Rectangle&, Color, optional<Color> outline = none);
  void drawFilledRectangle(int px, int py, int kx, int ky, Color color, optional<Color> outline = none);
  void drawLine(Vec2 from, Vec2 to, Color, double width);
  void drawPoint(Vec2 pos, Color color, int size);
  void drawViewObject(Vec2 pos, const ViewObject&, bool useSprite, Vec2 size);
  void drawViewObject(Vec2 pos, const ViewObject&, bool useSprite, double scale = 1, Color = Color::WHITE);
  void drawViewObject(Vec2 pos, const ViewObject&);
  void drawViewObject(Vec2 pos, ViewId, bool useSprite, double scale = 1, Color = Color::WHITE);
  void drawViewObject(Vec2 pos, ViewId, bool useSprite, Vec2 size, Color = Color::WHITE, SpriteOrientation = {});
  void drawViewObject(Vec2 pos, ViewId, Color = Color::WHITE);
  void drawAsciiBackground(ViewId, Rectangle bounds);
  void drawTile(Vec2 pos, const vector<TileCoord>&, Vec2 size, Color = Color::WHITE, SpriteOrientation orientation = {},
      optional<Color> secondColor = none, optional<double> scale = none);
  void setScissor(optional<Rectangle>, bool reset = false);
  void addQuad(const Rectangle&, Color);
  void drawAnimation(AnimationId, Vec2, double state, Vec2 squareSize, Dir orientation, Color);
  Vec2 getSize();
  Vec2 getWindowSize(); // size = window size / zoom
  Vec2 getMinResolution();

  void drawAndClearBuffer();
  void resize(int width, int height);
  bool pollEvent(Event&, EventType);
  bool pollEvent(Event&);
  void flushEvents(EventType);
  void waitEvent(Event&);
  Vec2 getMousePos();
  bool isKeypressed(SDL::SDL_Scancode);

  void setTopLayer();
  void popLayer();

  void startMonkey();
  bool isMonkey();

  void playVideo(const string& path, bool withAudio);

  static void putPixel(SDL::SDL_Surface*, Vec2, Color);
  void addTilesDirectory(const DirectoryPath&, Vec2 size);
  void setAnimationsDirectory(const DirectoryPath&);
  void loadAnimations();
  void makeScreenshot(const FilePath&, Rectangle bounds);
  void renderDeferredSprites();

  private:
  friend class Texture;
  optional<Texture> textTexture;
  optional<Texture> logoTexture;
  Renderer(const Renderer&);
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
  int getFont(FontId);
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
  vector<Rectangle> scissorStack;
  void loadTilesFromDir(const DirectoryPath&, Vec2 size, int setWidth);
  struct TileDirectory {
    DirectoryPath path;
    Vec2 size;
  };
  optional<DirectoryPath> animationDirectory;
  Clock* clock;
  TileSet* tileSet = nullptr;
  bool keypressed[SDL::SDL_NUM_SCANCODES] = {0};
  void updateKeypressed(const Event&);
};

