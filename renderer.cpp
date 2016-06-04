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
#include "renderer.h"
#include "view_object.h"
#include "tile.h"
#include "dirent.h"

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>

EnumMap<ColorId, Color> colors({
  {ColorId::WHITE, Color(255, 255, 255)},
  {ColorId::YELLOW, Color(250, 255, 0)},
  {ColorId::LIGHT_BROWN, Color(210, 150, 0)},
  {ColorId::ORANGE_BROWN, Color(250, 150, 0)},
  {ColorId::BROWN, Color(240, 130, 0)},
  {ColorId::DARK_BROWN, Color(100, 60, 0)},
  {ColorId::MAIN_MENU_ON, Color(255, 255, 255, 190)},
  {ColorId::MAIN_MENU_OFF, Color(255, 255, 255, 70)},
  {ColorId::LIGHT_GRAY, Color(150, 150, 150)},
  {ColorId::GRAY, Color(100, 100, 100)},
  {ColorId::ALMOST_GRAY, Color(102, 102, 102)},
  {ColorId::DARK_GRAY, Color(50, 50, 50)},
  {ColorId::ALMOST_BLACK, Color(20, 20, 20)},
  {ColorId::ALMOST_DARK_GRAY, Color(60, 60, 60)},
  {ColorId::BLACK, Color(0, 0, 0)},
  {ColorId::ALMOST_WHITE, Color(200, 200, 200)},
  {ColorId::GREEN, Color(0, 255, 0)},
  {ColorId::LIGHT_GREEN, Color(100, 255, 100)},
  {ColorId::DARK_GREEN, Color(0, 150, 0)},
  {ColorId::RED, Color(255, 0, 0)},
  {ColorId::LIGHT_RED, Color(255, 100, 100)},
  {ColorId::PINK, Color(255, 20, 147)},
  {ColorId::ORANGE, Color(255, 165, 0)},
  {ColorId::BLUE, Color(0, 0, 255)},
  {ColorId::NIGHT_BLUE, Color(0, 0, 20)},
  {ColorId::DARK_BLUE, Color(50, 50, 200)},
  {ColorId::LIGHT_BLUE, Color(100, 100, 255)},
  {ColorId::PURPLE, Color(160, 32, 240)},
  {ColorId::VIOLET, Color(120, 0, 255)},
  {ColorId::TRANSLUCENT_BLACK, Color(0, 0, 0)},
  {ColorId::TRANSPARENT, Color(0, 0, 0, 0)}
});

Color::Color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) : SDL_Color{r, g, b, a} {
}

Color::Color() : Color(0, 0, 0) {
}

Color Color::f(double r, double g, double b, double a) {
  return Color(r * 255, g * 255, b * 255, a * 255);
}

void Color::applyGl() const {
  glColor4f((float)r / 255, (float)g / 255, (float)b / 255, (float)a / 255);
}

Color Color::operator* (Color c2) {
  return Color(r * c2.r / 255, g * c2.g / 255, b * c2.b / 255, a * c2.a / 255);
}

Renderer::TileCoord::TileCoord(Vec2 p, int t) : pos(p), texNum(t) {
}

Renderer::TileCoord::TileCoord() : TileCoord(Vec2(0, 0), -1) {
}

Color transparency(const Color& color, int trans) {
  return Color{color.r, color.g, color.b, (Uint8)trans};
}

/*Text& Renderer::getTextObject() {
  static Text t1, t2;
  if (currentThreadId() == *renderThreadId)
    return t1;
  else
    return t2;
}*/

static void checkOpenglError() {
  auto error = glGetError();
  string s;
  CHECK(error == GL_NO_ERROR) << (int)error;
}

Texture::Texture(const string& fileName){
  SDL_Surface* image= IMG_Load(fileName.c_str());
  CHECK(image) << IMG_GetError();
  loadFrom(image);
  SDL_FreeSurface(image);
  path = fileName;
}

Texture::~Texture() {
  if (texId)
    glDeleteTextures(1, &(*texId));
}

static int totalTex = 0;

void Texture::loadFrom(SDL_Surface* image) {
  if (!texId) {
    texId = 0;
    glGenTextures(1, &(*texId));
    std::cout << "Total tex" << ++totalTex << std::endl;
  }
  checkOpenglError();
  glBindTexture(GL_TEXTURE_2D, (*texId));
  checkOpenglError();
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  checkOpenglError();
  int mode = GL_RGB;
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
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  checkOpenglError();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image ->h, 0, mode, GL_UNSIGNED_BYTE, image->pixels);
  checkOpenglError();
  size = Vec2(image->w, image->h);
}

Texture::Texture(const string& path, int px, int py, int w, int h) {
  SDL_Surface* image = IMG_Load(path.c_str());
  CHECK(image) << IMG_GetError();
  SDL_Rect offset;
  offset.x = 0;
  offset.y = 0;
  SDL_Rect src { px, py, w, h };
  SDL_Surface* sub = Renderer::createSurface(src.w, src.h);
  CHECK(sub) << SDL_GetError();
  CHECK(!SDL_BlitSurface(image, &src, sub, &offset)) << SDL_GetError();
  SDL_FreeSurface(image);
  loadFrom(sub);
  SDL_FreeSurface(sub);
  this->path = path;
}

Texture::Texture(SDL_Surface* surface) {
  loadFrom(surface);
}

Texture::Texture(Texture&& tex) {
  *this = std::move(tex);
}

Texture& Texture::operator = (Texture&& tex) {
  size = tex.size;
  texId = tex.texId;
  path = tex.path;
  tex.texId = none;
  return *this;
}

optional<Texture> Texture::loadMaybe(const string& path) {
  if (SDL_Surface* image= IMG_Load(path.c_str())) {
    Texture ret(image);
    SDL_FreeSurface(image);
    return std::move(ret);
  } else
    return none;
}

const Vec2& Texture::getSize() const {
  return size;
}

void Texture::addTexCoord(int x, int y) const {
  glTexCoord2f((float)x / size.x, (float)y / size.y);
}

void Texture::render(Vec2 a, Vec2 b, Vec2 p, Vec2 k, optional<Color> color, bool vFlip, bool hFlip) const {
  if (vFlip)
    swap(p.y, k.y);
  if (hFlip)
    swap(p.x, k.x);
  glBindTexture(GL_TEXTURE_2D, (*texId));
  checkOpenglError();
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  if (color)
    color->applyGl();
  else
    glColor3f(1, 1, 1);
  addTexCoord(p.x, p.y);
  glVertex2f(a.x, a.y);
  addTexCoord(k.x, p.y);
  glVertex2f(b.x, a.y);
  addTexCoord(k.x, k.y);
  glVertex2f(b.x, b.y);
  addTexCoord(p.x, k.y);
  glVertex2f(a.x, b.y);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  checkOpenglError();
}

int Renderer::getTextLength(const string& s, int size, FontId font) {
  int w, h;
  TTF_SizeUTF8(getFont(font, size), s.c_str(), &w, &h);
  return w;
}

TTF_Font* Renderer::getFont(Renderer::FontId id, int size) {
//  vector<FontSet>& fontSet = currentThreadId() == *renderThreadId ? fonts : fontsOtherThread;
  map<int, FontSet>& fontSet = fonts;
  if (!fontSet.count(size)) {
    FontSet set;
    loadFonts(fontPath, size, set);
    fontSet[size] = set;
  }
  switch (id) {
    case Renderer::TILE_FONT:
    case Renderer::TEXT_FONT: return NOTNULL(fontSet.at(size).textFont);
    case Renderer::SYMBOL_FONT: return NOTNULL(fontSet.at(size).symbolFont);
  }
}

void Renderer::drawText(FontId id, int size, Color color, int x, int y, const string& s, CenterType center) {
  if (!s.empty())
    addRenderElem([this, s, center, size, color, x, y, id] {
        int ox = 0;
        int oy = 0;
        int width, height;
        TTF_SizeUTF8(getFont(id, size), s.c_str(), &width, &height);
        switch (center) {
          case HOR:
            ox -= width / 2;
            break;
          case VER:
            oy -= height / 2;
            break;
          case HOR_VER:
            ox -= width / 2;
            oy -= height / 2;
            break;
          default:
            break;
        }
        SDL_Surface* res = TTF_RenderUTF8_Blended(getFont(id, size), s.c_str(), Color(255, 255, 255));
        CHECK(res) << TTF_GetError();
        if (!textTexture)
          textTexture.emplace(res);
        else
          textTexture->loadFrom(res);
        Vec2 p(ox + x, oy + y);
        textTexture->render(p, p + textTexture->getSize(), Vec2(0, 0), textTexture->getSize(), color);
        SDL_FreeSurface(res);
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
      drawFilledRectangle(pos, y + 23, pos + getTextLength(text.substr(ind, 1)), y + 25, colors[ColorId::GREEN]);
    }
  }
  drawText(color, x, y, text);
}

void Renderer::drawImage(int px, int py, const Texture& image, double scale, optional<Color> color) {
  Vec2 p(px, py);
  addRenderElem([this, p, &image, scale, color] {
    image.render(p, p + image.getSize() * scale, Vec2(0, 0), image.getSize(), color);
  });
}

void Renderer::drawImage(Rectangle target, Rectangle source, const Texture& image) {
  drawSprite(target.topLeft(), source.topLeft(), source.getSize(), image, target.getSize());
}

void Renderer::drawSprite(Vec2 pos, Vec2 stretchSize, const Texture& t) {
  drawSprite(pos, Vec2(0, 0), t.getSize(), t, stretchSize);
}

void Renderer::drawSprite(Vec2 pos, Vec2 source, Vec2 size, const Texture& t, optional<Vec2> targetSize,
    optional<Color> color, bool vFlip, bool hFlip) {
  addRenderElem([this, &t, pos, source, size, targetSize, color, vFlip, hFlip] {
      if (targetSize)
        t.render(pos, pos + *targetSize, source, source + size, color, vFlip, hFlip);
      else
        t.render(pos, pos + size, source, source + size, color, vFlip, hFlip);
  });
}

void Renderer::drawFilledRectangle(const Rectangle& t, Color color, optional<Color> outline) {
  addRenderElem([this, t, color, outline] {
    Vec2 a = t.topLeft();
    Vec2 b = t.bottomRight();
    if (outline) {
      glLineWidth(2);
      glBegin(GL_LINE_LOOP);
      outline->applyGl();
      glVertex2f(a.x, a.y);
      glVertex2f(b.x, a.y);
      glVertex2f(b.x, b.y);
      glVertex2f(a.x, b.y);
      glEnd();
      a += Vec2(2, 2);
      b -= Vec2(2, 2);
    }
    glBegin(GL_QUADS);
    color.applyGl();
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(a.x, b.y);
    glEnd();
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
      glScissor(s->left(), getSize().y - s->bottom(), s->width(), s->height());
      glEnable(GL_SCISSOR_TEST);
    }
    else
      glDisable(GL_SCISSOR_TEST);    
    scissor = s;
  }
}

void Renderer::addRenderElem(function<void()> f) {
  optional<Rectangle> thisScissor = scissor;
  f = [f, thisScissor, this] { setGlScissor(thisScissor); f(); };
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
  return Vec2(width / zoom, height / zoom);
}

bool Renderer::isFullscreen() {
  return fullscreen;
}

void Renderer::putPixel(SDL_Surface* surface, Vec2 pos, Color color) {
  Uint32 *pixels = (Uint32 *)surface->pixels; 
  pixels[ ( pos.y * surface->w ) + pos.x ] = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);;
}

void Renderer::setFullscreen(bool v) {
  fullscreen = v;
}

void Renderer::setFullscreenMode(int v) {
  fullscreenMode = v;
}

void Renderer::setZoom(int v) {
  zoom = v;
  initOpenGL();
}

void Renderer::initOpenGL() {
    bool success = true;
    GLenum error = GL_NO_ERROR;
    //Initialize Projection Matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport(0, 0, width, height);
    glOrtho(0.0, width / zoom, height / zoom, 0.0, -1.0, 1.0);
    CHECK(glGetError() == GL_NO_ERROR);
    //Initialize Modelview Matrix
        glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    CHECK(glGetError() == GL_NO_ERROR);
}

void Renderer::initialize() {
  CHECK(TTF_Init() == 0) << TTF_GetError();
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

void Renderer::loadFonts(const string& fontPath, int size, FontSet& fonts) {
  CHECK(fonts.textFont = TTF_OpenFont((fontPath + "/Lato-Bol.ttf").c_str(), size)) << TTF_GetError();
  CHECK(fonts.symbolFont = TTF_OpenFont((fontPath + "/Symbola.ttf").c_str(), size)) << TTF_GetError();
}

void Renderer::showError(const string& s) {
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", s.c_str(), window);
}

Renderer::Renderer(const string& title, Vec2 nominal, const string& fontP) : nominalSize(nominal), fontPath(fontP) {
  CHECK(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) >= 0) << SDL_GetError();
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
  CHECK(window = SDL_CreateWindow("KeeperRL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_OPENGL)) << SDL_GetError();
  CHECK(SDL_GL_CreateContext(window)) << SDL_GetError();
  SDL_GetWindowSize(window, &width, &height);
  initOpenGL();
}

Vec2 getOffset(Vec2 sizeDiff, double scale) {
  return Vec2(round(sizeDiff.x * scale * 0.5), round(sizeDiff.y * scale * 0.5));
}

Color Renderer::getBleedingColor(const ViewObject& object) {
  double bleeding = object.getAttribute(ViewObject::Attribute::WOUNDED).get_value_or(0);
  if (bleeding > 0)
    bleeding = 0.3 + bleeding * 0.7;
  if (object.hasModifier(ViewObject::Modifier::SPIRIT_DAMAGE))
    return Color(255, 255, 255, (Uint8)(80 + max(0., (1 - bleeding) * (255 - 80))));
  else
    return Color::f(1, max(0., 1 - bleeding), max(0., 1 - bleeding));
}

void Renderer::drawTile(Vec2 pos, TileCoord coord, Vec2 size, Color color, bool hFlip, bool vFlip) {
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
  if (vFlip) {
    sz.y *= -1;
    tileSize.y *= -1;
    coordPos.y -= sz.y;
  }
  if (vFlip) {
    sz.x *= -1;
    tileSize.x *= -1;
    coordPos.x -= sz.x;
  }
  drawSprite(pos + off, coordPos, sz, *tex, tileSize, color);
}

void Renderer::drawTile(Vec2 pos, TileCoord coord, double scale, Color color) {
  CHECK(coord.texNum >= 0 && coord.texNum < Renderer::tiles.size());
  Vec2 sz = Renderer::tileSize[coord.texNum];
  Vec2 off = getOffset(Renderer::nominalSize - sz, scale);
  if (sz.y > nominalSize.y)
    off.y *= 2;
  drawSprite(pos + off, coord.pos.mult(sz), sz, tiles.at(coord.texNum), sz * scale, color);
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

void Renderer::drawViewObject(Vec2 pos, ViewId id, bool useSprite, Vec2 size, Color color) {
  const Tile& tile = Tile::getTile(id, useSprite);
  if (tile.hasSpriteCoord())
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), size, color * tile.color);
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
    drawFilledRectangle(bounds, colors[ColorId::BLACK]);
}

const static string imageSuf = ".png";

bool Renderer::loadAltTilesFromDir(const string& path, Vec2 altSize) {
  altTileSize.push_back(altSize);
  return loadTilesFromDir(path, altTiles, altSize, 720 * altSize.x / tileSize.back().x);
}

bool Renderer::loadTilesFromDir(const string& path, Vec2 size) {
  tileSize.push_back(size);
  return loadTilesFromDir(path, tiles, size, 720);
}

SDL_Surface* Renderer::createSurface(int w, int h) {
  SDL_Surface* ret = SDL_CreateRGBSurface(0, w, h, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
  CHECK(ret) << SDL_GetError();
  return ret;
}

bool Renderer::loadTilesFromDir(const string& path, vector<Texture>& tiles, Vec2 size, int setWidth) {
  DIR* dir = opendir(path.c_str());
  if (!dir)
    return false;
  vector<string> files;
  while (dirent* ent = readdir(dir)) {
    string name(ent->d_name);
    Debug() << "Found " << name;
    if (endsWith(name, imageSuf))
      files.push_back(name);
  }
  int rowLength = setWidth / size.x;
  SDL_Surface* image = createSurface(setWidth, ((files.size() + rowLength - 1) / rowLength) * size.y);
  CHECK(image) << SDL_GetError();
  for (int i : All(files)) {
    SDL_Surface* im = IMG_Load((path + "/" + files[i]).c_str());
    CHECK(im) << files[i] << ": "<< IMG_GetError();
    CHECK(im->w == size.x && im->h == size.y) << files[i] << " has wrong size " << im->w << " " << im->h;
    SDL_Rect offset;
    offset.x = size.x * (i % rowLength);
    offset.y = size.y * (i / rowLength);
    SDL_BlitSurface(im, nullptr, image, &offset);
    CHECK(!tileCoords.count(files[i])) << "Duplicate name " << files[i];
    tileCoords[files[i].substr(0, files[i].size() - imageSuf.size())] =
        {{i % rowLength, i / rowLength}, int(tiles.size())};
    SDL_FreeSurface(im);
  }
  tiles.push_back(Texture(image));
  SDL_FreeSurface(image);
  return true;
}

Renderer::TileCoord Renderer::getTileCoord(const string& name) {
  CHECK(tileCoords.count(name)) << "Tile not found " << name;
  return tileCoords.at(name);
}

Vec2 Renderer::getNominalSize() const {
  return nominalSize;
}

void Renderer::drawAndClearBuffer() {
  for (int i : All(renderList)) {
    for (auto& elem : renderList[i])
      elem();
    renderList[i].clear();
  }
  setGlScissor(none);
  SDL_GL_SwapWindow(window);  
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0, 0.0, 0.0, 0.0);

}

void Renderer::resize(int w, int h) {
  width = w;
  height = h;
  initOpenGL();
}

Event Renderer::getRandomEvent() {
  FAIL << "Unimpl";
  return SDL_Event();
/*  Uint8 ty
  Event ret;
  ret.type = type;
  int modProb = 5;
  switch (type) {
    case Event::KeyPressed:
      ret.key = {Keyboard::Key(Random.get(int(Keyboard::Key::KeyCount))), Random.roll(modProb),
          Random.roll(modProb), Random.roll(modProb), Random.roll(modProb) };
      break;
    case Event::MouseButtonReleased:
    case Event::MouseButtonPressed:
      ret.mouseButton = { Random.choose({Mouse::Left, Mouse::Right}), Random.get(getSize().x),
        Random.get(getSize().y) };
      break;
    case Event::MouseMoved:
      ret.mouseMove = { Random.get(getSize().x), Random.get(getSize().y) };
      break;
    default: return getRandomEvent();
  }
  return ret;*/
}

bool Renderer::pollEventOrFromQueue(Event& ev) {
  if (!eventQueue.empty()) {
    ev = eventQueue.front();
    eventQueue.pop_front();
    return true;
  } else if (SDL_PollEvent(&ev)) {
    zoomMousePos(ev);
    considerMouseMoveEvent(ev);
    return true;
  } else
    return false;
}

void Renderer::considerMouseMoveEvent(Event& ev) {
  if (ev.type == SDL_MOUSEMOTION)
    mousePos = Vec2(ev.motion.x, ev.motion.y);
}

bool Renderer::pollEvent(Event& ev) {
  CHECK(currentThreadId() == *renderThreadId);
  if (monkey) {
    if (Random.roll(2))
      return false;
    ev = getRandomEvent();
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
  switch (ev.type) {
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
      ev.button.x /= zoom;
      ev.button.y /= zoom;
      break;
    case SDL_MOUSEMOTION:
      ev.motion.x /= zoom;
      ev.motion.y /= zoom;
      break;
    case SDL_MOUSEWHEEL:
      ev.wheel.x /= zoom;
      ev.wheel.y /= zoom;
      break;
    default:
      break;
  }
}

void Renderer::waitEvent(Event& ev) {
  if (monkey) {
    ev = getRandomEvent();
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
