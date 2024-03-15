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

#include "stdafx.h"
// #include "dirent.h"
#include "extern/lodepng.h"

#include "renderer.h"
#include "view_object.h"
#include "tile.h"

#include "fontstash.h"
#include "sdl_event_generator.h"
#include "clock.h"
#include "gzstream.h"
#include "opengl.h"
#include "tileset.h"
#include "steam_input.h"

void Renderer::renderDeferredSprites() {
  static vector<SDL::GLfloat> vertices;
  static vector<SDL::GLfloat> texCoords;
  static vector<SDL::GLfloat> colors;
  vertices.clear();
  texCoords.clear();
  colors.clear();
  auto addVertex = [&](Vec2 v, int texX, int texY, Vec2 texSize, Color color) {
    vertices.push_back(v.x);
    vertices.push_back(v.y);
    texCoords.push_back(((float)texX) / texSize.x);
    texCoords.push_back(((float)texY) / texSize.y);
    colors.push_back(((float) color.r) / 255);
    colors.push_back(((float) color.g) / 255);
    colors.push_back(((float) color.b) / 255);
    colors.push_back(((float) color.a) / 255);
  };
  for (auto& elem : deferredSprites) {
    auto add = [&](Vec2 v, int texX, int texY, const DeferredSprite& draw) {
      addVertex(v, texX, texY, draw.realSize, draw.color.value_or(Color::WHITE));
    };
    add(elem.a, elem.p.x, elem.p.y, elem);
    add(elem.b, elem.k.x, elem.p.y, elem);
    add(elem.c, elem.k.x, elem.k.y, elem);
    add(elem.a, elem.p.x, elem.p.y, elem);
    add(elem.c, elem.k.x, elem.k.y, elem);
    add(elem.d, elem.p.x, elem.k.y, elem);
  }
  if (!vertices.empty()) {
    CHECK_OPENGL_ERROR();
    SDL::glBindTexture(GL_TEXTURE_2D, *currentTexture);
    SDL::glEnable(GL_TEXTURE_2D);
    SDL::glEnableClientState(GL_VERTEX_ARRAY);
    SDL::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    SDL::glEnableClientState(GL_COLOR_ARRAY);
    SDL::glColorPointer(4, GL_FLOAT, 0, colors.data());
    SDL::glVertexPointer(2, GL_FLOAT, 0, vertices.data());
    SDL::glTexCoordPointer(2, GL_FLOAT, 0, texCoords.data());
    SDL::glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
    vertices.clear();
    texCoords.clear();
    colors.clear();

    SDL::glDisableClientState(GL_VERTEX_ARRAY);
    SDL::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    SDL::glDisableClientState(GL_COLOR_ARRAY);
    SDL::glDisable(GL_TEXTURE_2D);
    CHECK_OPENGL_ERROR();
  }
  deferredSprites.clear();
}

void Renderer::drawSprite(const Texture& t, Vec2 topLeft, Vec2 bottomRight, Vec2 p, Vec2 k, optional<Color> color) {
  drawSprite(t, topLeft, Vec2(bottomRight.x, topLeft.y), bottomRight, Vec2(topLeft.x, bottomRight.y), p, k, color);
}

void Renderer::drawSprite(const Texture& t, Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 p, Vec2 k, optional<Color> color) {
  if (currentTexture && currentTexture != t.getTexId())
    renderDeferredSprites();
  currentTexture = t.getTexId();
  CHECK(currentTexture);
  deferredSprites.push_back({a, b, c, d, p, k, t.getRealSize(), color});
}

static float sizeConv(int size) {
  return 1.15 * (float)size;
}

int Renderer::getTextLength(const string& s, int size, FontId font) {
  return getTextSize(s, size, font).x;
}

static optional<int> getMapFontOffset(char c) {
  switch (c) {
    case 'A': return 0;
    case 'B': return 14;
    case 'C': return 28;
    case 'D': return 42;
    case 'E': return 56;
    case 'F': return 70;
    case 'G': return 84;
    case 'H': return 98;
    case 'I': return 112;
    case 'J': return 122;
    case 'K': return 136;
    case 'L': return 150;
    case 'M': return 162;
    case 'N': return 180;
    case 'O': return 194;
    case 'P': return 208;
    case 'Q': return 222;
    case 'R': return 236;
    case 'S': return 250;
    case 'T': return 264;
    case 'U': return 278;
    case 'V': return 292;
    case 'W': return 306;
    case 'X': return 324;
    case 'Y': return 338;
    case 'Z': return 352;
    case 'a': return 366;
    case 'b': return 378;
    case 'c': return 390;
    case 'd': return 400;
    case 'e': return 412;
    case 'f': return 424;
    case 'g': return 434;
    case 'h': return 446;
    case 'i': return 458;
    case 'j': return 464;
    case 'k': return 472;
    case 'l': return 484;
    case 'm': return 490;
    case 'n': return 508;
    case 'o': return 520;
    case 'p': return 532;
    case 'q': return 544;
    case 'r': return 556;
    case 's': return 566;
    case 't': return 578;
    case 'u': return 588;
    case 'v': return 600;
    case 'w': return 612;
    case 'x': return 626;
    case 'y': return 638;
    case 'z': return 650;
    case 'z' + 1: return 660;
  }
  return none;
}

static optional<int> getMapFontWidth(char c) {
  if (c == ' ')
    return 8;
  if (c >= 'A' && c <= 'Y')
    return *getMapFontOffset(c + 1) - *getMapFontOffset(c);
  if (c == 'Z')
    return *getMapFontOffset('a') - *getMapFontOffset(c);
  if (c >= 'a' && c <= 'z')
    return *getMapFontOffset(c + 1) - *getMapFontOffset(c);
  return none;
}

Vec2 Renderer::getTextSize(const string& s, int size, FontId id) {
  if (id == FontId::MAP_FONT) {
    auto ret = 0;
    for (auto c : s)
      if (auto width = getMapFontWidth(c))
        ret += *width - 2;
    return Vec2(ret, 22);
  }
  if (s.empty())
    return Vec2(0, 0);
  float minx, maxx, miny, maxy;
  int font = getFont(id);
  sth_dim_text(fontStash, font, sizeConv(size), s.c_str(), &minx, &miny, &maxx, &maxy);
  float height;
  sth_vmetrics(fontStash, font, sizeConv(size), nullptr, nullptr, &height);
  return Vec2(maxx - minx, height);
}

int Renderer::getFont(FontId id) {
//  vector<FontSet>& fontSet = currentThreadId() == *renderThreadId ? fonts : fontsOtherThread;
  FontSet& fontSet = fonts;
  switch (id) {
    case FontId::TILE_FONT:
    case FontId::TEXT_FONT: return fontSet.textFont;
    case FontId::SYMBOL_FONT: return fontSet.symbolFont;
    case FontId::MAP_FONT: return 0;
  }
}

void Renderer::drawText(FontId id, int size, Color color, Vec2 pos, const string& s, CenterType center) {
  renderDeferredSprites();
  if (id == FontId::MAP_FONT) {
    for (auto c : s) {
      if (auto offset = getMapFontOffset(c))
        drawSprite(pos, Vec2(*offset, 0), Vec2(*getMapFontWidth(c), 22), *mapFontTexture, none, color);
      if (auto width = getMapFontWidth(c))
        pos.x += *width - 2;
    }
    return;
  }
  if (!s.empty()) {
    int ox = 0;
    int oy = 0;
    Vec2 dim = getTextSize(s, size, id);
    switch (center) {
      case HOR:
        ox -= dim.x / 2;
        break;
      case VER:
        oy -= dim.y / 2;
        break;
      case HOR_VER:
        ox -= dim.x / 2;
        oy -= dim.y / 2;
        break;
      default:
        break;
    }
    sth_begin_draw(fontStash);
    glColor(color);
    sth_draw_text(fontStash, getFont(id), sizeConv(size), ox + pos.x, oy + pos.y + (dim.y * 0.9), s.c_str(), nullptr);
    sth_end_draw(fontStash);
  }
}

void Renderer::drawText(Color color, Vec2 pos, const char* c, CenterType center, int size) {
  drawText(FontId::TEXT_FONT, size, color, pos, c, center);
}

void Renderer::drawText(Color color, Vec2 pos, const string& c, CenterType center, int size) {
  drawText(FontId::TEXT_FONT, size, color, pos, c, center);
}

void Renderer::drawTextWithHotkey(Color color, Vec2 pos, const string& text, char key) {
  if (key) {
    int ind = lowercase(text).find(key);
    if (ind != string::npos) {
      int posHotkey = pos.x + getTextLength(text.substr(0, ind));
      drawFilledRectangle(posHotkey, pos.y + 23, posHotkey + getTextLength(text.substr(ind, 1)), pos.y + 25, Color::GREEN);
    }
  }
  drawText(color, pos, text);
}

void Renderer::drawImage(int px, int py, const Texture& image, double scale, optional<Color> color) {
  Vec2 p(px, py);
  drawSprite(image, p, p + image.getSize() * scale, Vec2(0, 0), image.getSize(), color);
}

void Renderer::drawImage(Rectangle target, Rectangle source, const Texture& image) {
  drawSprite(target.topLeft(), source.topLeft(), source.getSize(), image, target.getSize());
}

void Renderer::drawSprite(Vec2 pos, Vec2 stretchSize, const Texture& t) {
  drawSprite(pos, Vec2(0, 0), t.getSize(), t, stretchSize);
}

static Vec2 rotate(Vec2 pos, Vec2 origin, double x, double y) {
  Vec2 v = pos - origin;
  return origin + Vec2(v.x * x - v.y * y, v.x * y + v.y * x);
}

void Renderer::drawAnimation(AnimationId id, Vec2 pos, double state, Vec2 squareSize, Dir orientation, Color color) {
  if (auto& animInfo = animations[id]) {
    int zoomLevel = squareSize.y / nominalSize;
    int frame = int(animInfo->numFrames * state);
    int width = animInfo->tex.getSize().x / animInfo->numFrames;
    Vec2 size(width, animInfo->tex.getSize().y);
    drawSprite(pos - size * zoomLevel / 2, Vec2(frame * width, 0), size, animInfo->tex, size * zoomLevel, color,
        SpriteOrientation(Vec2(rotate(rotate(orientation))), false));
  }
}

void Renderer::drawSprite(Vec2 pos, Vec2 source, Vec2 size, const Texture& t,
    optional<Vec2> targetSize, optional<Color> color, SpriteOrientation orientation) {
  Vec2 a = pos;
  Vec2 c = targetSize ? (a + *targetSize) : (a + size);
  Vec2 b(c.x, a.y);
  Vec2 d(a.x, c.y);
  if (orientation.horizontalFlip) {
    swap(a, b);
    swap(c, d);
  }
  if (orientation.x != 1 || orientation.y != 0) {
    Vec2 mid = a + c;
    a = rotate(a * 2, mid, orientation.x, orientation.y) / 2;
    b = rotate(b * 2, mid, orientation.x, orientation.y) / 2;
    c = rotate(c * 2, mid, orientation.x, orientation.y) / 2;
    d = rotate(d * 2, mid, orientation.x, orientation.y) / 2;
  }
  drawSprite(t, a, b, c, d, source, source + size, color);
}

void Renderer::drawFilledRectangle(const Rectangle& t, Color color, optional<Color> outline) {
  renderDeferredSprites();
  Vec2 a = t.topLeft();
  Vec2 b = t.bottomRight();
  if (outline) {
    SDL::glLineWidth(2);
    SDL::glBegin(GL_LINE_LOOP);
    glColor(*outline);
    SDL::glVertex2f(a.x + 1.5f, a.y + 1.0f);
    SDL::glVertex2f(b.x - 0.5f, a.y + 1.0f);
    SDL::glVertex2f(b.x - 0.5f, b.y - 0.5f);
    SDL::glVertex2f(a.x + 1.5f, b.y - 0.5f);
    SDL::glEnd();
    a += Vec2(2, 2);
    b -= Vec2(1, 1);
  }
  SDL::glBegin(GL_QUADS);
  glColor(color);
  SDL::glVertex2f(a.x, a.y);
  SDL::glVertex2f(b.x, a.y);
  SDL::glVertex2f(b.x, b.y);
  SDL::glVertex2f(a.x, b.y);
  SDL::glEnd();
}

void Renderer::drawLine(Vec2 from, Vec2 to, Color color, double width) {
  renderDeferredSprites();
  SDL::glBegin(GL_QUADS);
  double dx = to.x - from.x;
  double dy = to.y - from.y;
  double length = sqrt(dx * dx + dy * dy);
  dx /= length;
  dy /= length;
  glColor(color);
  width /= 2;
  SDL::glVertex2d(from.x + dy * width, from.y - dx * width);
  SDL::glVertex2d(to.x + dy * width, to.y - dx * width);
  SDL::glVertex2d(to.x - dy * width, to.y + dx * width);
  SDL::glVertex2d(from.x - dy * width, from.y + dx * width);
  SDL::glEnd();
}

void Renderer::drawFilledRectangle(int px, int py, int kx, int ky, Color color, optional<Color> outline) {
  drawFilledRectangle(Rectangle(px, py, kx, ky), color, outline);
}

void Renderer::drawPoint(Vec2 pos, Color color, int size) {
  SDL::glPointSize(size);
  SDL::glBegin(GL_POINTS);
  glColor(color);
  SDL::glVertex2f(pos.x, pos.y);
  SDL::glEnd();
}

void Renderer::addQuad(const Rectangle& r, Color color) {
  drawFilledRectangle(r.left(), r.top(), r.right(), r.bottom(), color);
}

void Renderer::setScissor(optional<Rectangle> s, bool reset) {
  renderDeferredSprites();
  auto applyScissor = [&] (Rectangle rect) {
    int zoom = getZoom();
    SDL::glScissor(rect.left() * zoom, (getSize().y - rect.bottom()) * zoom,
        rect.width() * zoom, rect.height() * zoom);
    SDL::glEnable(GL_SCISSOR_TEST);
  };
  if (s) {
    Rectangle rect = *s;
    if (!scissorStack.empty() && !reset)
      rect = rect.intersection(scissorStack.back());
    applyScissor(rect);
    scissorStack.push_back(rect);
  }
  else {
    if (!scissorStack.empty())
      scissorStack.pop_back();
    if (!scissorStack.empty())
      applyScissor(scissorStack.back());
    else
      SDL::glDisable(GL_SCISSOR_TEST);
  }
}

void Renderer::setTopLayer() {
  renderDeferredSprites();
  SDL::glPushMatrix();
  SDL::glTranslated(0, 0, 1);
  SDL::glDisable(GL_SCISSOR_TEST);
  CHECK_OPENGL_ERROR();
}

void Renderer::popLayer() {
  renderDeferredSprites();
  SDL::glPopMatrix();
  if (!scissorStack.empty())
    SDL::glEnable(GL_SCISSOR_TEST);
  CHECK_OPENGL_ERROR();
}

Vec2 Renderer::getSize() {
  return Vec2(width / getZoom(), height / getZoom());
}

Vec2 Renderer::getWindowSize() {
  return Vec2(width, height);
}

bool Renderer::isFullscreen() {
  return fullscreen;
}

void Renderer::putPixel(SDL::SDL_Surface* surface, Vec2 pos, Color color) {
  SDL::Uint32 *pixels = (SDL::Uint32 *)surface->pixels;
  pixels[ ( pos.y * surface->w ) + pos.x ] = SDL::SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);;
}

void Renderer::setFullscreen(bool v) {
  fullscreen = v;
  CHECK(SDL::SDL_SetWindowFullscreen(window, v ? SDL::SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0) << SDL::SDL_GetError();
  SDL_GL_GetDrawableSize(window, &width, &height);
  initOpenGL();
}

void Renderer::setFullscreenMode(int v) {
  fullscreenMode = v;
}

static const Vec2 minResolution = Vec2(800, 600);

Vec2 Renderer::getMinResolution() {
  return minResolution;
}

int Renderer::getZoom() {
  if (zoom == 1 || (width / zoom >= minResolution.x && height / zoom >= minResolution.y))
    return zoom;
  else
    return 1;
}

void Renderer::setZoom(int v) {
  zoom = v;
  initOpenGL();
}

void Renderer::enableCustomCursor(bool state) {
  cursorEnabled = state;
  reloadCursors();
}

void Renderer::initOpenGL() {
  setupOpenglView(width, height, getZoom());
  SDL::glEnable(GL_BLEND);
  SDL::glEnable(GL_TEXTURE_2D);
  SDL::glEnable(GL_DEPTH_TEST);
  SDL::glDepthFunc(GL_LEQUAL);
  SDL::glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  CHECK(SDL::glGetError() == GL_NO_ERROR);
  reloadCursors();
}

SDL::SDL_Surface* Renderer::loadScaledSurface(const FilePath& path, double scale) {
  if (auto surface = SDL::IMG_Load(path.getPath())) {
    if (scale == 1)
      return surface;
    if (auto scaled = Texture::createSurface(surface->w * scale, surface->h * scale)) {
      SDL::SDL_SetSurfaceBlendMode(surface, SDL::SDL_BLENDMODE_NONE);
      CHECK(!SDL_BlitScaled(surface, nullptr, scaled, nullptr)) << SDL::IMG_GetError();
      SDL_FreeSurface(surface);
      return scaled;
    }
    SDL_FreeSurface(surface);
  }
  return nullptr;
}

void Renderer::reloadCursors() {
  if (!cursorEnabled) {
    SDL_SetCursor(originalCursor);
    cursor = cursorClicked = nullptr;
  } else {
    if (auto surface = loadScaledSurface(cursorPath, getZoom()))
      cursor = SDL_CreateColorCursor(surface, 0, 0);
    if (auto surface = loadScaledSurface(clickedCursorPath, getZoom()))
      cursorClicked = SDL_CreateColorCursor(surface, 0, 0);
    if (cursor)
      SDL_SetCursor(cursor);
  }
}

void Renderer::initialize() {
  if (!renderThreadId)
    renderThreadId = currentThreadId();
  else
    CHECK(currentThreadId() == *renderThreadId);
  initOpenGL();
}

void Renderer::loadFonts(const DirectoryPath& fontPath, FontSet& fonts) {
  CHECK(fontStash = sth_create(512, 512)) << "Error initializing fonts";
  auto textFont = fontPath.file("Lato-Bol.ttf");
  auto symbolFont = fontPath.file("Symbola.ttf");
  fonts.textFont = sth_add_font(fontStash, textFont.getPath());
  fonts.symbolFont = sth_add_font(fontStash, symbolFont.getPath());
  CHECK(fonts.textFont >= 0) << "Error loading " << textFont;
  CHECK(fonts.symbolFont >= 0) << "Error loading " << symbolFont;
}

void Renderer::showError(const string& s) {
  SDL_ShowSimpleMessageBox(SDL::SDL_MESSAGEBOX_ERROR, "Error", s.c_str(), window);
}

TileSet& Renderer::getTileSet() {
  return *tileSet;
}

void Renderer::setTileSet(TileSet* s) {
  tileSet = s;
}

void Renderer::setVsync(bool on) {
  SDL::SDL_GL_SetSwapInterval(on ? 1 : 0);
}

void Renderer::setFpsLimit(int fps) {
  fpsLimit = fps;
}

Renderer::Renderer(Clock* clock, MySteamInput* i, const string& title, const DirectoryPath& fontPath,
    const FilePath& cursorP, const FilePath& clickedCursorP, const FilePath& iconPath, const FilePath& mapFontPath)
    : cursorPath(cursorP), clickedCursorPath(clickedCursorP),
      clock(clock), steamInput(i) {
  CHECK(SDL::SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) >= 0) << SDL::SDL_GetError();
  SDL::SDL_GL_SetAttribute(SDL::SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
  SDL::SDL_GL_SetAttribute(SDL::SDL_GL_CONTEXT_MINOR_VERSION, 1 );
  CHECK(window = SDL::SDL_CreateWindow("KeeperRL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1200, 720,
    SDL::SDL_WINDOW_RESIZABLE | SDL::SDL_WINDOW_SHOWN | SDL::SDL_WINDOW_MAXIMIZED | SDL::SDL_WINDOW_OPENGL)) << SDL::SDL_GetError();
  CHECK(SDL::SDL_GL_CreateContext(window)) << SDL::SDL_GetError();
  SDL_SetWindowMinimumSize(window, minResolution.x, minResolution.y);
  SDL::SDL_Event ev;
  while(SDL_PollEvent(&ev)){}
  SDL::SDL_GL_GetDrawableSize(window, &width, &height);
  setVsync(true);
  originalCursor = SDL::SDL_GetCursor();
  initOpenGL();
  loadFonts(fontPath, fonts);
  auto icon = SDL::IMG_Load(iconPath.getPath());
  SDL_SetWindowIcon(window, icon);
  mapFontTexture.emplace(std::move(*Texture::loadMaybe(mapFontPath))); // work around compiler bug?
}

Vec2 getOffset(Vec2 sizeDiff, double scale) {
  return Vec2(round(sizeDiff.x * scale * 0.5), round(sizeDiff.y * scale * 0.5));
}

void Renderer::drawTile(Vec2 pos, const vector<TileCoord>& coords, Vec2 size, Color color, SpriteOrientation orientation,
    optional<Color> secondColor, optional<double> scale) {
  if (coords.empty())
    return;
  auto drawCoord = [&size, scale, this, pos, orientation] (const TileCoord& coord, Color color) {
    auto sizeDiff = coord.size.x == 8 ? Vec2(0, 0) : Vec2(nominalSize, nominalSize) - coord.size;
    Vec2 off = scale ? getOffset(sizeDiff, *scale) : sizeDiff.mult(size) / (nominalSize * 2);
    Vec2 tileSize = scale ? coord.size * *scale : coord.size.mult(size) / nominalSize;
    if (coord.size.y > nominalSize)
      off.y -= 3 * size.y / nominalSize;
    drawSprite(pos + off, coord.pos.mult(coord.size), coord.size, *coord.texture, tileSize, color, orientation);
  };
  if (secondColor && coords.size() > 1) {
    drawCoord(coords[0], color);
    if (secondColor != Color::WHITE)
      drawCoord(coords[1], color * *secondColor);
  } else {
    auto frame = clock->getRealMillis().count() / 200;
    drawCoord(coords[frame % coords.size()], color);
  }
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, Color color) {
  drawViewObject(pos, id, true, 1, color);
}

void Renderer::drawViewObject(Vec2 pos, ViewIdList id, bool useSprite, double scale, Color color) {
  for (int i : All(id)) {
    const Tile& tile = tileSet->getTile(id[i], useSprite);
    if (tile.hasSpriteCoord()) {
      optional<Color> colorVariant;
      if (!tile.animated)
        colorVariant = id[i].getColor();
      auto thisPos = pos;
      auto coord = tile.getSpriteCoord(DirSet::fullSet());
      if (tile.weaponOrigin && i < id.size() - 1) {
        auto creatureTile = tileSet->getTile(id[i + 1], useSprite);
        if (creatureTile.weaponOrigin) {
          auto weaponSize = coord[0].size;
          auto creatureSize = creatureTile.getSpriteCoord(DirSet::fullSet())[0].size;
          thisPos -= (*tile.weaponOrigin - *creatureTile.weaponOrigin + (creatureSize - weaponSize) / 2) * scale;
        }
      }
      drawTile(thisPos, coord, Vec2(), color * tile.color, {}, colorVariant, scale);
    } else
      drawText(tile.symFont ? FontId::SYMBOL_FONT : FontId::TEXT_FONT, 20 * scale, color * tile.color,
          pos + Vec2(scale * nominalSize / 2, 0), tile.text, HOR);
  }
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, bool useSprite, double scale, Color color) {
  const Tile& tile = tileSet->getTile(id, useSprite);
  if (tile.hasSpriteCoord()) {
    optional<Color> colorVariant;
    if (!tile.animated)
      colorVariant = id.getColor();
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), Vec2(), color * tile.color, {}, colorVariant, scale);
  } else
    drawText(tile.symFont ? FontId::SYMBOL_FONT : FontId::TEXT_FONT, 20 * scale, color * tile.color,
        pos + Vec2(scale * nominalSize / 2, 0), tile.text, HOR);
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, bool useSprite, Vec2 size, Color color, SpriteOrientation orient) {
  const Tile& tile = tileSet->getTile(id, useSprite);
  if (tile.hasSpriteCoord()) {
    optional<Color> colorVariant;
    if (!tile.animated)
      colorVariant = id.getColor();
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), size, color * tile.color, orient, colorVariant);
  } else
    drawText(tile.symFont ? FontId::SYMBOL_FONT : FontId::TEXT_FONT, size.y, color * tile.color, pos, tile.text);
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object, bool useSprite, Vec2 size) {
  drawViewObject(pos, object.id(), useSprite, size, Color::WHITE);
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object, bool useSprite, double scale, Color color) {
  const Tile& tile = tileSet->getTile(object.id(), useSprite);
  if (object.weaponViewId && tile.weaponOrigin) {
    const Tile& weaponTile = tileSet->getTile(*object.weaponViewId, useSprite);
    if (weaponTile.weaponOrigin) {
      auto size = tile.getSpriteCoord(DirSet::fullSet())[0].size;
      auto weaponSize = weaponTile.getSpriteCoord(DirSet::fullSet())[0].size;
      drawViewObject(pos - (*weaponTile.weaponOrigin - *tile.weaponOrigin + (size - weaponSize) / 2) * scale, *object.weaponViewId,
          useSprite, scale, Color::WHITE);
    }
  }
  drawViewObject(pos, object.id(), useSprite, scale, color);
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object) {
  drawViewObject(pos, object.id(), Color::WHITE);
}

void Renderer::drawAsciiBackground(ViewId id, Rectangle bounds) {
  if (!tileSet->getTile(id, true).hasSpriteCoord())
    drawFilledRectangle(bounds, Color::BLACK);
}

void Renderer::loadAnimations() {
  if (animationDirectory) {
    for (auto id : ENUM_ALL(AnimationId))
      animations[id] = AnimationInfo { Texture(animationDirectory->file(getFileName(id))), getNumFrames(id)};
  }
}

void Renderer::setAnimationsDirectory(const DirectoryPath& path) {
  animationDirectory = path;
}

void Renderer::drawAndClearBuffer() {
  #ifdef USE_STEAMWORKS
  if (steamInput)
    steamInput->runFrame();
  #endif
  renderDeferredSprites();
  CHECK_OPENGL_ERROR();
  if (fpsLimit) {
    uint64_t end = SDL::SDL_GetPerformanceCounter();
    float elapsedMs = (end - frameStart) / (float)SDL::SDL_GetPerformanceFrequency() * 1000.0f;
    float sleepMs = 1000.0f / fpsLimit - elapsedMs;
    if (sleepMs > 0.0)
      SDL::SDL_Delay(sleepMs);
  }
  SDL::SDL_GL_SwapWindow(window);
  CHECK_OPENGL_ERROR();
  frameStart = SDL::SDL_GetPerformanceCounter();
  SDL::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  SDL::glClearColor(0.0, 0.0, 0.0, 0.0);
  CHECK_OPENGL_ERROR();
}

void Renderer::resize(int w, int h) {
  SDL_GL_GetDrawableSize(window, &width, &height);
  initOpenGL();
}

bool Renderer::isKeypressed(SDL::SDL_Scancode key) {
  return keypressed[key];
}

void Renderer::updateKeypressed(const Event& ev) {
  if (ev.type == SDL::SDL_KEYDOWN)
    keypressed[ev.key.keysym.scancode] = true;
  else if (ev.type == SDL::SDL_KEYUP)
    keypressed[ev.key.keysym.scancode] = false;
}

bool Renderer::pollEventOrFromQueue(Event& ev) {
  if (!eventQueue.empty()) {
    ev = eventQueue.front();
    eventQueue.pop_front();
    updateKeypressed(ev);
    return true;
  } else if (SDL_PollEvent(&ev)) {
    zoomMousePos(ev);
    considerMouseMoveEvent(ev);
    considerMouseCursorAnim(ev);
    updateKeypressed(ev);
    return true;
  } else
    return false;
}

void Renderer::considerMouseCursorAnim(Event& ev) {
  if (ev.type == SDL::SDL_MOUSEBUTTONDOWN) {
    if (cursorClicked)
      SDL_SetCursor(cursorClicked);
  } else
  if (ev.type == SDL::SDL_MOUSEBUTTONUP) {
    if (cursor)
      SDL_SetCursor(cursor);
  }
}

void Renderer::considerMouseMoveEvent(Event& ev) {
  if (ev.type == SDL::SDL_MOUSEMOTION)
    mousePos = Vec2(ev.motion.x, ev.motion.y);
}

bool Renderer::pollEvent(Event& ev) {
  CHECK(currentThreadId() == *renderThreadId);
  #ifdef USE_STEAMWORKS
  if (steamInput)
    if (auto e = steamInput->getEvent()) {
      ev.type = SDL::SDL_KEYDOWN;
      ev.key = SDL::SDL_KeyboardEvent {
          ev.type,
          0, 0,
          SDL_PRESSED,
          0, 0, 0,
          SDL::SDL_Keysym { SDL::SDL_Scancode{} , (SDL::SDL_Keycode) *e }
      };
      return true;
    }
  #endif
  if (monkey) {
    if (Random.roll(2))
      return pollEventOrFromQueue(ev);
    ev = SdlEventGenerator::getRandom(Random, getSize());
    return true;
  } else
    return pollEventOrFromQueue(ev);
}

Vec2 Renderer::getDiscreteJoyPos(ControllerJoy joy) {
  Vec2 ret;
  #ifdef USE_STEAMWORKS
  if (steamInput) {
    auto pos = steamInput->getJoyPos(joy);
    if (pos.first <= -0.5)
      ret.x = -1;
    else if (pos.first >= 0.5)
      ret.x = 1;
    if (pos.second <= -0.5)
      ret.y = 1;
    else if (pos.second >= 0.5)
      ret.y = -1;
  }
  #endif
  return ret;
}

MySteamInput* Renderer::getSteamInput() {
  return steamInput;
}

void Renderer::flushEvents(EventType type) {
  Event ev;
  while (SDL_PollEvent(&ev)) {
    zoomMousePos(ev);
    considerMouseMoveEvent(ev);
    if (ev.type != type)
      eventQueue.push_back(ev);
  }
}

void Renderer::zoomMousePos(Event& ev) {
  auto zoom = getZoom();
  switch (ev.type) {
    case SDL::SDL_MOUSEBUTTONUP:
    case SDL::SDL_MOUSEBUTTONDOWN:
      ev.button.x /= zoom;
      ev.button.y /= zoom;
      break;
    case SDL::SDL_MOUSEMOTION:
      ev.motion.x /= zoom;
      ev.motion.y /= zoom;
      break;
    default:
      break;
  }
}

void Renderer::waitEvent(Event& ev) {
  if (monkey) {
    ev = SdlEventGenerator::getRandom(Random, getSize());
    return;
  } else {
    if (!eventQueue.empty()) {
      ev = eventQueue.front();
      eventQueue.pop_front();
    } else {
      SDL_WaitEvent(&ev);
      zoomMousePos(ev);
      considerMouseMoveEvent(ev);
    }
  }
}

Vec2 Renderer::getMousePos() {
  if (monkey)
    return Vec2(Random.get(getSize().x), Random.get(getSize().y));
  else
    return mousePos;
}

void Renderer::startMonkey() {
  monkey = true;
}

bool Renderer::isMonkey() {
  return monkey;
}

SDL::SDL_Surface* flipVert(SDL::SDL_Surface* sfc) {
   auto result = SDL::SDL_CreateRGBSurface(sfc->flags, sfc->w, sfc->h,
     sfc->format->BytesPerPixel * 8, sfc->format->Rmask, sfc->format->Gmask,
     sfc->format->Bmask, sfc->format->Amask);
   CHECK(result);
   std::uint8_t* pixels = (std::uint8_t*) sfc->pixels;
   std::uint8_t* rpixels = (std::uint8_t*) result->pixels;
   std::uint32_t pitch = sfc->pitch;
   std::uint32_t pxlength = pitch*sfc->h;
   for(auto line = 0; line < sfc->h; ++line) {
     std::uint32_t pos = line * pitch;
     memcpy(rpixels + pos, pixels + pxlength - pos - pitch, pitch);
   }
   return result;
}

void Renderer::makeScreenshot(const FilePath& path, Rectangle bounds) {
  auto image = SDL::SDL_CreateRGBSurface(SDL_SWSURFACE, bounds.width(), bounds.height(), 24, 0x000000FF, 0x0000FF00, 0x00FF0000, 0);
  SDL::glReadBuffer(GL_FRONT);
  SDL::glReadPixels(bounds.left(), height - bounds.bottom(), bounds.width(), bounds.height(), GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
  auto inverted = flipVert(image);
  unsigned error = lodepng::encode(path.getPath(), (unsigned char*)inverted->pixels, bounds.width(), bounds.height(), LCT_RGB);
  USER_CHECK(!error) << "encoder error " << error << ": "<< lodepng_error_text(error);
  SDL_FreeSurface(image);
  SDL_FreeSurface(inverted);
}

void playfile(const char *fname, SDL::SDL_Window* screen, Renderer&, float volume);

void Renderer::playVideo(const string& path, int volume) {
  playfile(path.data(), window, *this, ((float)volume) / 100);
}
