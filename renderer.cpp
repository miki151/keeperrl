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
#include "dirent.h"

#include "renderer.h"
#include "view_object.h"
#include "tile.h"

#include "fontstash.h"
#include "sdl_event_generator.h"
#include "clock.h"

Color Color::WHITE(255, 255, 255);
Color Color::YELLOW(250, 255, 0);
Color Color::LIGHT_BROWN(210, 150, 0);
Color Color::ORANGE_BROWN(250, 150, 0);
Color Color::BROWN(240, 130, 0);
Color Color::DARK_BROWN(100, 60, 0);
Color Color::MAIN_MENU_ON(255, 255, 255, 190);
Color Color::MAIN_MENU_OFF(255, 255, 255, 70);
Color Color::LIGHT_GRAY(150, 150, 150);
Color Color::GRAY(100, 100, 100);
Color Color::ALMOST_GRAY(102, 102, 102);
Color Color::DARK_GRAY(50, 50, 50);
Color Color::ALMOST_BLACK(20, 20, 20);
Color Color::ALMOST_DARK_GRAY(60, 60, 60);
Color Color::BLACK(0, 0, 0);
Color Color::ALMOST_WHITE(200, 200, 200);
Color Color::GREEN(0, 255, 0);
Color Color::LIGHT_GREEN(100, 255, 100);
Color Color::DARK_GREEN(0, 150, 0);
Color Color::RED(255, 0, 0);
Color Color::LIGHT_RED(255, 100, 100);
Color Color::PINK(255, 20, 147);
Color Color::ORANGE(255, 165, 0);
Color Color::BLUE(0, 0, 255);
Color Color::NIGHT_BLUE(0, 0, 20);
Color Color::DARK_BLUE(50, 50, 200);
Color Color::LIGHT_BLUE(100, 100, 255);
Color Color::PURPLE(160, 32, 240);
Color Color::VIOLET(120, 0, 255);
Color Color::TRANSLUCENT_BLACK(0, 0, 0);
Color Color::TRANSPARENT(0, 0, 0, 0);

Color::Color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) : SDL_Color{r, g, b, a} {
}

Color Color::transparency(int trans) {
  return Color(r, g, b, (Uint8)trans);
}

Color::Color() : Color(0, 0, 0) {
}

Color Color::f(double r, double g, double b, double a) {
  return Color(r * 255, g * 255, b * 255, a * 255);
}

void Color::applyGl() const {
  SDL::glColor4f((float)r / 255, (float)g / 255, (float)b / 255, (float)a / 255);
}

Color Color::operator* (Color c2) {
  return Color(r * c2.r / 255, g * c2.g / 255, b * c2.b / 255, a * c2.a / 255);
}

Renderer::TileCoord::TileCoord(Vec2 p, int t) : pos(p), texNum(t) {
}

Renderer::TileCoord::TileCoord() : TileCoord(Vec2(0, 0), -1) {
}

static void checkOpenglError() {
  auto error = SDL::glGetError();
  string s;
  CHECK(error == GL_NO_ERROR) << (int)error;
}

Texture::Texture(const FilePath& fileName) : path(fileName) {
  SDL::SDL_Surface* image= SDL::IMG_Load(fileName.getPath());
  CHECK(image) << SDL::IMG_GetError();
  if (auto error = loadFromMaybe(image))
    FATAL << "Couldn't load image: " << fileName << ". Error code " << toString(*error);
  SDL::SDL_FreeSurface(image);
}

Texture::~Texture() {
  if (texId)
    SDL::glDeleteTextures(1, &(*texId));
}

static int totalTex = 0;

// Some graphic cards need power of two sized textures, so we create a larger texture to contain the original one.
SDL::SDL_Surface* Renderer::createPowerOfTwoSurface(SDL::SDL_Surface* image) {
  int w = 1;
  int h = 1;
  while (w < image->w)
    w *= 2;
  while (h < image->h)
    h *= 2;
  if (w == image->w && h == image->h)
    return image;
  auto ret = createSurface(w, h);
  SDL::SDL_Rect dst {0, 0, image->w, image->h };
  SDL_BlitSurface(image, nullptr, ret, &dst);
  // fill the rest of the texture as well, which 'kind-of' solves the problem with repeating textures.
  dst = {image->w, 0, 0, 0};
  SDL_BlitSurface(image, nullptr, ret, &dst);
  dst = {image->w, image->h, 0, 0};
  SDL_BlitSurface(image, nullptr, ret, &dst);
  dst = {0, image->h, 0, 0};
  SDL_BlitSurface(image, nullptr, ret, &dst);
  return ret;
}

optional<SDL::GLenum> Texture::loadFromMaybe(SDL::SDL_Surface* imageOrig) {
  if (!texId) {
    texId = 0;
    SDL::glGenTextures(1, &(*texId));
  }
  checkOpenglError();
  SDL::glBindTexture(GL_TEXTURE_2D, (*texId));
  checkOpenglError();
  SDL::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  SDL::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  SDL::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  SDL::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  checkOpenglError();
  int mode = GL_RGB;
  auto image = Renderer::createPowerOfTwoSurface(imageOrig);
  if (image->format->BytesPerPixel == 4) {
    if (image->format->Rmask == 0x000000ff)
      mode = GL_RGBA;
    else
      mode = GL_BGRA;
  } else {
    if (image->format->Rmask == 0x000000ff)
      mode = GL_RGB;
    else
      mode = GL_BGR;
  }
  SDL::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  SDL::glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  SDL::glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  SDL::glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  checkOpenglError();
  SDL::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image ->h, 0, mode, GL_UNSIGNED_BYTE, image->pixels);
  size = Vec2(imageOrig->w, imageOrig->h);
  realSize = Vec2(image->w, image->h);
  if (image != imageOrig)
    SDL::SDL_FreeSurface(image);
  auto error = SDL::glGetError();
  if (error != GL_NO_ERROR)
    return error;
  return none;
}

Texture::Texture(const FilePath& filename, int px, int py, int w, int h) : path(filename) {
  SDL::SDL_Surface* image = SDL::IMG_Load(path->getPath());
  CHECK(image) << SDL::IMG_GetError();
  SDL::SDL_Rect offset;
  offset.x = 0;
  offset.y = 0;
  SDL::SDL_Rect src { px, py, w, h };
  SDL::SDL_Surface* sub = Renderer::createSurface(src.w, src.h);
  CHECK(sub) << SDL::SDL_GetError();
  CHECK(!SDL_BlitSurface(image, &src, sub, &offset)) << SDL::SDL_GetError();
  SDL::SDL_FreeSurface(image);
  if (auto error = loadFromMaybe(sub))
    FATAL << "Couldn't load image: " << *path << ". Error code " << toString(*error);
  SDL::SDL_FreeSurface(sub);
}

Texture::Texture(SDL::SDL_Surface* surface) {
  CHECK(!loadFromMaybe(surface));
}

Texture::Texture(Texture&& tex) {
  *this = std::move(tex);
}

Texture& Texture::operator = (Texture&& tex) {
  size = tex.size;
  realSize = tex.realSize;
  texId = tex.texId;
  path = tex.path;
  tex.texId = none;
  return *this;
}

optional<Texture> Texture::loadMaybe(const FilePath& path) {
  if (SDL::SDL_Surface* image = SDL::IMG_Load(path.getPath())) {
    Texture ret;
    bool ok = !ret.loadFromMaybe(image);
    SDL::SDL_FreeSurface(image);
    if (ok) {
      ret.path = path;
      return std::move(ret);
    }
  }
  return none;
}

const Vec2& Texture::getSize() const {
  return size;
}

void Texture::addTexCoord(int x, int y) const {
  SDL::glTexCoord2f((float)x / realSize.x, (float)y / realSize.y);
}

Texture::Texture() {
}

void Texture::render(Vec2 topLeft, Vec2 bottomRight, Vec2 p, Vec2 k, optional<Color> color) const {
  render(topLeft, Vec2(bottomRight.x, topLeft.y), bottomRight, Vec2(topLeft.x, bottomRight.y), p, k, color);
}

void Texture::render(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 p, Vec2 k, optional<Color> color) const {
  SDL::glBindTexture(GL_TEXTURE_2D, (*texId));
  checkOpenglError();
  SDL::glEnable(GL_TEXTURE_2D);
  SDL::glBegin(GL_QUADS);
  if (color)
    color->applyGl();
  else
    SDL::glColor3f(1, 1, 1);
  addTexCoord(p.x, p.y);
  SDL::glVertex2f(a.x, a.y);
  addTexCoord(k.x, p.y);
  SDL::glVertex2f(b.x, b.y);
  addTexCoord(k.x, k.y);
  SDL::glVertex2f(c.x, c.y);
  addTexCoord(p.x, k.y);
  SDL::glVertex2f(d.x, d.y);
  SDL::glEnd();
  SDL::glDisable(GL_TEXTURE_2D);
  checkOpenglError();
}

static float sizeConv(int size) {
  return 1.15 * (float)size;
}

int Renderer::getTextLength(const string& s, int size, FontId font) {
  return getTextSize(s, size, font).x;
}

Vec2 Renderer::getTextSize(const string& s, int size, FontId id) {
  if (s.empty())
    return Vec2(0, 0);
  float minx, maxx, miny, maxy;
  int font = getFont(id);
  sth_dim_text(fontStash, font, sizeConv(size), s.c_str(), &minx, &miny, &maxx, &maxy);
  float height;
  sth_vmetrics(fontStash, font, sizeConv(size), nullptr, nullptr, &height);
  return Vec2(maxx - minx, height);
}

int Renderer::getFont(Renderer::FontId id) {
//  vector<FontSet>& fontSet = currentThreadId() == *renderThreadId ? fonts : fontsOtherThread;
  FontSet& fontSet = fonts;
  switch (id) {
    case Renderer::TILE_FONT:
    case Renderer::TEXT_FONT: return fontSet.textFont;
    case Renderer::SYMBOL_FONT: return fontSet.symbolFont;
  }
}

void Renderer::drawText(FontId id, int size, Color color, int x, int y, const string& s, CenterType center) {
  if (!s.empty())
    addRenderElem([this, s, center, size, color, x, y, id] {
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
        color.applyGl();
        sth_draw_text(fontStash, getFont(id), sizeConv(size), ox + x, oy + y + (dim.y * 0.9), s.c_str(), nullptr);
        sth_end_draw(fontStash);
    });
}

void Renderer::drawText(Color color, int x, int y, const char* c, CenterType center, int size) {
  drawText(TEXT_FONT, size, color, x, y, c, center);
}

void Renderer::drawText(Color color, int x, int y, const string& c, CenterType center, int size) {
  drawText(TEXT_FONT, size, color, x, y, c, center);
}

void Renderer::drawTextWithHotkey(Color color, int x, int y, const string& text, char key) {
  if (key) {
    int ind = lowercase(text).find(key);
    if (ind != string::npos) {
      int pos = x + getTextLength(text.substr(0, ind));
      drawFilledRectangle(pos, y + 23, pos + getTextLength(text.substr(ind, 1)), y + 25, Color::GREEN);
    }
  }
  drawText(color, x, y, text);
}

void Renderer::drawImage(int px, int py, const Texture& image, double scale, optional<Color> color) {
  Vec2 p(px, py);
  addRenderElem([p, &image, scale, color] {
    image.render(p, p + image.getSize() * scale, Vec2(0, 0), image.getSize(), color);
  });
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

void Renderer::drawSprite(Vec2 pos, Vec2 source, Vec2 size, const Texture& t,
    optional<Vec2> targetSize, optional<Color> color, SpriteOrientation orientation) {
  addRenderElem([&t, pos, source, size, targetSize, color, orientation] {
      Vec2 a = pos;
      Vec2 c = targetSize ? (a + *targetSize) : (a + size);
      Vec2 b(c.x, a.y);
      Vec2 d(a.x, c.y);
      if (orientation.horizontalFlip) {
        swap(a, b);
        swap(c, d);
      }
      if (orientation.x != 1 || orientation.y != 0) {
        Vec2 mid = (a + c) / 2;
        a = rotate(a, mid, orientation.x, orientation.y);
        b = rotate(b, mid, orientation.x, orientation.y);
        c = rotate(c, mid, orientation.x, orientation.y);
        d = rotate(d, mid, orientation.x, orientation.y);
      }
      t.render(a, b, c, d, source, source + size, color);
  });
}

void Renderer::drawFilledRectangle(const Rectangle& t, Color color, optional<Color> outline) {
  addRenderElem([t, color, outline] {
    Vec2 a = t.topLeft();
    Vec2 b = t.bottomRight();
    if (outline) {
      SDL::glLineWidth(2);
      SDL::glBegin(GL_LINE_LOOP);
      outline->applyGl();
      SDL::glVertex2f(a.x + 1.5f, a.y + 1.5f);
      SDL::glVertex2f(b.x - 0.5f, a.y + 1.5f);
      SDL::glVertex2f(b.x - 0.5f, b.y - 0.5f);
      SDL::glVertex2f(a.x + 1.5f, b.y - 0.5f);
      SDL::glEnd();
      a += Vec2(2, 2);
      b -= Vec2(2, 2);
    }
    SDL::glBegin(GL_QUADS);
    color.applyGl();
    SDL::glVertex2f(a.x, a.y);
    SDL::glVertex2f(b.x, a.y);
    SDL::glVertex2f(b.x, b.y);
    SDL::glVertex2f(a.x, b.y);
    SDL::glEnd();
  });
}

void Renderer::drawFilledRectangle(int px, int py, int kx, int ky, Color color, optional<Color> outline) {
  drawFilledRectangle(Rectangle(px, py, kx, ky), color, outline);
}

void Renderer::addQuad(const Rectangle& r, Color color) {
  drawFilledRectangle(r.left(), r.top(), r.right(), r.bottom(), color);
}

void Renderer::drawQuads() {
/*  if (!quads.empty()) {
    vector<Vertex>& quadsTmp = quads;
    addRenderElem([this, quadsTmp] { display.draw(&quadsTmp[0], quadsTmp.size(), sf::Quads); });
    quads.clear();
  }*/
}

void Renderer::setScissor(optional<Rectangle> s) {
  scissor = s;
}

void Renderer::setGlScissor(optional<Rectangle> s) {
  if (s != scissor) {
    if (s) {
      int zoom = getZoom();
      SDL::glScissor(s->left() * zoom, (getSize().y - s->bottom()) * zoom, s->width() * zoom, s->height() * zoom);
      SDL::glEnable(GL_SCISSOR_TEST);
    }
    else
      SDL::glDisable(GL_SCISSOR_TEST);
    scissor = s;
  }
}

void Renderer::addRenderElem(function<void()> f) {
  if (currentLayer == 0) {
    auto thisScissor = scissor;
    f = [f, thisScissor, this] { setGlScissor(thisScissor); f(); };
  } else
    f = [f, this] { setGlScissor(none); f(); };
  renderList[currentLayer].push_back(f);
}

void Renderer::setTopLayer() {
  layerStack.push(currentLayer);
  currentLayer = 1;
}

void Renderer::popLayer() {
  currentLayer = layerStack.top();
  layerStack.pop();
}

Vec2 Renderer::getSize() {
  return Vec2(width / getZoom(), height / getZoom());
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
  initOpenGL();
}

void Renderer::setFullscreenMode(int v) {
  fullscreenMode = v;
}

const Vec2 minResolution = Vec2(800, 600);

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
    bool success = true;
    SDL::GLenum error = GL_NO_ERROR;
    //Initialize Projection Matrix
    SDL::glMatrixMode( GL_PROJECTION );
    SDL::glLoadIdentity();
    SDL::glViewport(0, 0, width, height);
    SDL::glOrtho(0.0, width / getZoom(), height / getZoom(), 0.0, -1.0, 1.0);
    CHECK(SDL::glGetError() == GL_NO_ERROR);
    //Initialize Modelview Matrix
    SDL::glMatrixMode( GL_MODELVIEW );
    SDL::glLoadIdentity();
    SDL::glEnable(GL_BLEND);
    SDL::glEnable(GL_TEXTURE_2D);
    SDL::glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    CHECK(SDL::glGetError() == GL_NO_ERROR);
    reloadCursors();
}

SDL::SDL_Surface* Renderer::loadScaledSurface(const FilePath& path, double scale) {
  if (auto surface = SDL::IMG_Load(path.getPath())) {
    if (scale == 1)
      return surface;
    if (auto scaled = createSurface(surface->w * scale, surface->h * scale)) {
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


vector<string> Renderer::getFullscreenResolutions() {
  return {};
}

void Renderer::printSystemInfo(ostream& out) {
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

void Renderer::setVsync(bool on) {
  SDL::SDL_GL_SetSwapInterval(on ? 1 : 0);
}

Renderer::Renderer(const string& title, Vec2 nominal, const DirectoryPath& fontPath, const FilePath& cursorP,
    const FilePath& clickedCursorP) : nominalSize(nominal), cursorPath(cursorP), clickedCursorPath(clickedCursorP) {
  CHECK(SDL::SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) >= 0) << SDL::SDL_GetError();
  SDL::SDL_GL_SetAttribute(SDL::SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
  SDL::SDL_GL_SetAttribute(SDL::SDL_GL_CONTEXT_MINOR_VERSION, 1 );
  CHECK(window = SDL::SDL_CreateWindow("KeeperRL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1200, 720,
    SDL::SDL_WINDOW_RESIZABLE | SDL::SDL_WINDOW_SHOWN | SDL::SDL_WINDOW_MAXIMIZED | SDL::SDL_WINDOW_OPENGL)) << SDL::SDL_GetError();
  CHECK(SDL::SDL_GL_CreateContext(window)) << SDL::SDL_GetError();
  SDL_SetWindowMinimumSize(window, minResolution.x, minResolution.y);
  SDL_GetWindowSize(window, &width, &height);
  setVsync(true);
  originalCursor = SDL::SDL_GetCursor();
  initOpenGL();
  loadFonts(fontPath, fonts);
}

Vec2 getOffset(Vec2 sizeDiff, double scale) {
  return Vec2(round(sizeDiff.x * scale * 0.5), round(sizeDiff.y * scale * 0.5));
}

Color Renderer::getBleedingColor(const ViewObject& object) {
  double bleeding = object.getAttribute(ViewObject::Attribute::WOUNDED).value_or(0);
  if (bleeding > 0)
    bleeding = 0.3 + bleeding * 0.7;
  if (object.hasModifier(ViewObject::Modifier::SPIRIT_DAMAGE))
    return Color(255, 255, 255, (Uint8)(80 + max(0., (1 - bleeding) * (255 - 80))));
  else
    return Color::f(1, max(0., 1 - bleeding), max(0., 1 - bleeding));
}

void Renderer::drawTile(Vec2 pos, TileCoord coord, Vec2 size, Color color, SpriteOrientation orientation) {
  CHECK(coord.texNum >= 0 && coord.texNum < Renderer::tiles.size());
  Texture* tex = &tiles[coord.texNum];
  Vec2 sz = tileSize[coord.texNum];
  Vec2 off = (nominalSize -  sz).mult(size).div(Renderer::nominalSize * 2);
  Vec2 tileSize = sz.mult(size).div(nominalSize);
  if (sz.y > nominalSize.y)
    off.y *= 2;
  if (altTileSize.size() > coord.texNum && size == altTileSize[coord.texNum]) {
    sz = size;
    tex = &altTiles[coord.texNum];
  }
  Vec2 coordPos = coord.pos.mult(sz);
  drawSprite(pos + off, coordPos, sz, *tex, tileSize, color, orientation);
}

void Renderer::drawTile(Vec2 pos, TileCoord coord, double scale, Color color) {
  CHECK(coord.texNum >= 0 && coord.texNum < Renderer::tiles.size());
  Vec2 sz = Renderer::tileSize[coord.texNum];
  Vec2 off = getOffset(Renderer::nominalSize - sz, scale);
  if (sz.y > nominalSize.y)
    off.y *= 2;
  drawSprite(pos + off, coord.pos.mult(sz), sz, tiles[coord.texNum], sz * scale, color);
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, Color color) {
  const Tile& tile = Tile::getTile(id);
  if (tile.hasSpriteCoord())
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), 1, color * tile.color);
  else
    drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 20, tile.color, pos.x + nominalSize.x / 2,
        pos.y, tile.text, HOR);
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, bool useSprite, double scale, Color color) {
  const Tile& tile = Tile::getTile(id, useSprite);
  if (tile.hasSpriteCoord())
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), scale, color * tile.color);
  else
    drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 20 * scale, color * tile.color,
        pos.x + scale * nominalSize.x / 2, pos.y, tile.text, HOR);
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, bool useSprite, Vec2 size, Color color, SpriteOrientation orient) {
  const Tile& tile = Tile::getTile(id, useSprite);
  if (tile.hasSpriteCoord())
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), size, color * tile.color, orient);
  else
    drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, size.y, color * tile.color, pos.x, pos.y,
        tile.text);
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object, bool useSprite, Vec2 size) {
  drawViewObject(pos, object.id(), useSprite, size, getBleedingColor(object));
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object, bool useSprite, double scale, Color color) {
  drawViewObject(pos, object.id(), useSprite, scale, getBleedingColor(object) * color);
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object) {
  drawViewObject(pos, object.id(), getBleedingColor(object));
}

void Renderer::drawAsciiBackground(ViewId id, Rectangle bounds) {
  if (!Tile::getTile(id, true).hasSpriteCoord())
    drawFilledRectangle(bounds, Color::BLACK);
}

bool Renderer::loadAltTilesFromDir(const DirectoryPath& path, Vec2 altSize) {
  altTileSize.push_back(altSize);
  return loadTilesFromDir(path, altTiles, altSize, 720 * altSize.x / tileSize.back().x);
}

bool Renderer::loadTilesFromDir(const DirectoryPath& path, Vec2 size) {
  tileSize.push_back(size);
  return loadTilesFromDir(path, tiles, size, 720);
}

SDL::SDL_Surface* Renderer::createSurface(int w, int h) {
  SDL::SDL_Surface* ret = SDL::SDL_CreateRGBSurface(0, w, h, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
  CHECK(ret) << "Failed to creature surface " << w << ":" << h << ": " << SDL::SDL_GetError();
  return ret;
}

bool Renderer::loadTilesFromDir(const DirectoryPath& path, vector<Texture>& tiles, Vec2 size, int setWidth) {
  const static string imageSuf = ".png";
  auto files = path.getFiles().filter([](const FilePath& f) { return f.hasSuffix(imageSuf);});
  int rowLength = setWidth / size.x;
  SDL::SDL_Surface* image = createSurface(setWidth, ((files.size() + rowLength - 1) / rowLength) * size.y);
  SDL::SDL_SetSurfaceBlendMode(image, SDL::SDL_BLENDMODE_NONE);
  CHECK(image) << SDL::SDL_GetError();
  for (int i : All(files)) {
    SDL::SDL_Surface* im = SDL::IMG_Load(files[i].getPath());
    SDL::SDL_SetSurfaceBlendMode(im, SDL::SDL_BLENDMODE_NONE);
    CHECK(im) << files[i] << ": "<< SDL::IMG_GetError();
    CHECK(im->w == size.x && im->h == size.y) << files[i] << " has wrong size " << im->w << " " << im->h;
    SDL::SDL_Rect offset;
    offset.x = size.x * (i % rowLength);
    offset.y = size.y * (i / rowLength);
    SDL_BlitSurface(im, nullptr, image, &offset);
    string fileName = files[i].getFileName();
    string spriteName = fileName.substr(0, fileName.size() - imageSuf.size());
    CHECK(!tileCoords.count(spriteName)) << "Duplicate name " << spriteName;
    tileCoords[spriteName] = {{i % rowLength, i / rowLength}, int(tiles.size())};
    SDL::SDL_FreeSurface(im);
  }
  tiles.push_back(Texture(image));
  SDL::SDL_FreeSurface(image);
  return true;
}

Renderer::TileCoord Renderer::getTileCoord(const string& name) {
  CHECK(tileCoords.count(name)) << "Tile not found " << name;
  return tileCoords.at(name);
}

Vec2 Renderer::getNominalSize() const {
  return nominalSize;
}

static void measureLatency() {
  static milliseconds last;
  static long maxLat = 0;
  static int counter = 0;
  auto curTime = Clock::getRealMillis();
  auto latency = (curTime - last).count();
  if (latency > maxLat)
    maxLat = latency;
  if (latency > 50)
    std::cout << "Draw buffer " << latency << std::endl;
  if (++counter > 100) {
    std::cout << "Max latency " << maxLat << std::endl;
    maxLat = 0;
    counter = 0;
  }
  last = curTime;
  auto state = Clock::getRealMillis().count() % 500;
  SDL::glBegin(GL_QUADS);
  Color(255, 255, 255).applyGl();
  SDL::glVertex2f(0, 0);
  SDL::glVertex2f(2 * state, 0);
  SDL::glVertex2f(2 * state, 30);
  SDL::glVertex2f(0, 30);
  SDL::glEnd();
}

void Renderer::drawAndClearBuffer() {
  for (int i : All(renderList)) {
    for (auto& elem : renderList[i])
      elem();
    renderList[i].clear();
  }
  setGlScissor(none);
  SDL::SDL_GL_SwapWindow(window);
  SDL::glClear(GL_COLOR_BUFFER_BIT);
  SDL::glClearColor(0.0, 0.0, 0.0, 0.0);

}

void Renderer::resize(int w, int h) {
  width = w;
  height = h;
  initOpenGL();
}

bool Renderer::pollEventOrFromQueue(Event& ev) {
  if (!eventQueue.empty()) {
    ev = eventQueue.front();
    eventQueue.pop_front();
    return true;
  } else if (SDL_PollEvent(&ev)) {
    zoomMousePos(ev);
    considerMouseMoveEvent(ev);
    considerMouseCursorAnim(ev);
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
  if (monkey) {
    if (Random.roll(2))
      return pollEventOrFromQueue(ev);
    ev = SdlEventGenerator::getRandom(Random, getSize());
    return true;
  } else 
    return pollEventOrFromQueue(ev);
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
    case SDL::SDL_MOUSEWHEEL:
      ev.wheel.x /= zoom;
      ev.wheel.y /= zoom;
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
