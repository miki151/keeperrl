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

using namespace sf;

EnumMap<ColorId, Color> colors;

Renderer::TileCoord::TileCoord(Vec2 p, int t) : pos(p), texNum(t) {
}

Renderer::TileCoord::TileCoord() : TileCoord(Vec2(0, 0), -1) {
}

Color transparency(const Color& color, int trans) {
  return Color(color.r, color.g, color.b, trans);
}

Text& Renderer::getTextObject() {
  static Text t1, t2;
  if (currentThreadId() == *renderThreadId)
    return t1;
  else
    return t2;
}

int Renderer::getUnicodeLength(String s, FontId font) {
  Text& t = getTextObject();
  t.setFont(getFont(font));
  t.setCharacterSize(textSize);
  t.setString(s);
  return t.getLocalBounds().width;
}

int Renderer::getTextLength(string s) {
  return getUnicodeLength(toUnicode(s), FontId::TEXT_FONT);
}

Font& Renderer::getFont(Renderer::FontId id) {
  FontSet& fontSet = currentThreadId() == *renderThreadId ? fonts : fontsOtherThread;
  switch (id) {
    case Renderer::TEXT_FONT: return fontSet.textFont;
    case Renderer::TILE_FONT: return fontSet.tileFont;
    case Renderer::SYMBOL_FONT: return fontSet.symbolFont;
  }
}

void Renderer::drawText(FontId id, int size, Color color, int x, int y, String s, CenterType center) {
  addRenderElem([this, s, center, size, color, x, y, id] {
      int ox = 0;
      int oy = 0;
      static Text t;
      t.setFont(getFont(id));
      t.setCharacterSize(size);
      t.setString(s);
      sf::FloatRect bounds = t.getLocalBounds();
      switch (center) {
        case HOR:
          ox -= bounds.left + bounds.width / 2;
          break;
        case VER:
          oy -= bounds.top + bounds.height / 2;
          break;
        case HOR_VER:
          ox -= bounds.left + bounds.width / 2;
          oy -= bounds.top + bounds.height / 2;
          break;
        default:
          break;
      }
      t.setPosition(x + ox, y + oy);
      t.setColor(color);
      display.draw(t);
  });
}

String Renderer::toUnicode(const string& s) {
  std::basic_string<sf::Uint32> utf32;
  sf::Utf8::toUtf32(s.begin(), s.end(), std::back_inserter(utf32));
  return utf32;
}

void Renderer::drawText(Color color, int x, int y, string s, CenterType center, int size) {
  drawText(TEXT_FONT, size, color, x, y, toUnicode(s), center);
}

void Renderer::drawText(Color color, int x, int y, const char* c, CenterType center, int size) {
  drawText(TEXT_FONT, size, color, x, y, String(c), center);
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

void Renderer::drawImage(int px, int py, const Texture& image, double scale) {
  drawImage(px, py, px + image.getSize().x * scale, py + image.getSize().y * scale, image, scale);
}

void Renderer::drawImage(Rectangle target, Rectangle source, const Texture& image) {
  drawSprite(target.getTopLeft(), source.getTopLeft(), source.getSize(), image, target.getSize());
}

void Renderer::drawImage(int px, int py, int kx, int ky, const Texture& t, double scale) {
  addRenderElem([this, &t, px, py, kx, ky, scale] {
      static Sprite s;
      s.setPosition(px, py);
      s.setTexture(t);
      s.setTextureRect(sf::IntRect(0, 0, (kx - px) / scale, (ky - py) / scale));
      s.setScale(scale, scale);
      display.draw(s);
  });
}

void Renderer::drawSprite(Vec2 pos, Vec2 spos, Vec2 size, const Texture& t, optional<Color> color,
    optional<Vec2> stretchSize) {
  if (stretchSize)
    drawSprite(pos, spos, size, t, *stretchSize, color);
  else
    drawSprite(pos, spos, size, t, Vec2(-1, -1), color);
}

void Renderer::drawSprite(Vec2 pos, Vec2 stretchSize, const Texture& t) {
  drawSprite(pos, Vec2(0, 0), Vec2(t.getSize().x, t.getSize().y), t, stretchSize);
}

void Renderer::drawSprite(Vec2 pos, Vec2 source, Vec2 size, const Texture& t, Vec2 targetSize,
    optional<Color> color) {
  addRenderElem([this, &t, pos, source, size, targetSize, color] {
      static Sprite s;
      s.setTexture(t);
      s.setTextureRect(sf::IntRect(source.x, source.y, size.x, size.y));
      s.setPosition(pos.x, pos.y);
      if (color)
        s.setColor(*color);
      else
        s.setColor(sf::Color(255, 255, 255));
      if (targetSize.x != -1)
        s.setScale(double(targetSize.x) / size.x, double(targetSize.y) / size.y);
      else
        s.setScale(1, 1);
      display.draw(s);   
  });
}

void Renderer::drawFilledRectangle(const Rectangle& t, Color color, optional<Color> outline) {
  addRenderElem([this, t, color, outline] {
      static RectangleShape r;
      r.setSize(Vector2f(t.getW(), t.getH()));
      r.setPosition(t.getPX(), t.getPY());
      r.setFillColor(color);
      if (outline) {
        r.setOutlineThickness(-2);
        r.setOutlineColor(*outline);
      } else
        r.setOutlineThickness(0);
      display.draw(r);
  });
}

void Renderer::drawFilledRectangle(int px, int py, int kx, int ky, Color color, optional<Color> outline) {
  drawFilledRectangle(Rectangle(px, py, kx, ky), color, outline);
}

Vector2f getV(Vec2 v) {
  return {float(v.x), float(v.y)};
}

void Renderer::addQuad(const Rectangle& r, Color color) {
  quads.push_back(Vertex(getV(r.getTopLeft()), color));
  quads.push_back(Vertex(getV(r.getTopRight()), color));
  quads.push_back(Vertex(getV(r.getBottomRight()), color));
  quads.push_back(Vertex(getV(r.getBottomLeft()), color));
}

void Renderer::drawQuads() {
  if (!quads.empty()) {
    vector<Vertex>& quadsTmp = quads;
    addRenderElem([this, quadsTmp] { display.draw(&quadsTmp[0], quadsTmp.size(), sf::Quads); });
    quads.clear();
  }
}

void Renderer::addRenderElem(function<void()> f) {
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
  return Vec2(display.getSize().x / zoom, display.getSize().y / zoom);
}

bool Renderer::isFullscreen() {
  return fullscreen;
}

static vector<VideoMode> getResolutions() {
  static VideoMode desktopMode = sf::VideoMode::getDesktopMode();
  return filter(sf::VideoMode::getFullscreenModes(),
      [&] (const VideoMode& m) { return m.bitsPerPixel == desktopMode.bitsPerPixel; });
}

void Renderer::updateResolution() {
  display.setView(sf::View(sf::FloatRect(0, 0, display.getSize().x / zoom, display.getSize().y / zoom)));
}

void Renderer::setFullscreen(bool v) {
  fullscreen = v;
}

void Renderer::setFullscreenMode(int v) {
  fullscreenMode = v;
}

void Renderer::setZoom(int v) {
  zoom = v;
  updateResolution();
}

void Renderer::initialize() {
  if (!renderThreadId)
    renderThreadId = currentThreadId();
  else
    CHECK(currentThreadId() == *renderThreadId);
  CHECK(!getResolutions().empty()) << sf::VideoMode::getFullscreenModes().size() << " " <<
      int(sf::VideoMode::getDesktopMode().bitsPerPixel);
  CHECK(fullscreenMode >= 0 && fullscreenMode < getResolutions().size()) <<
      fullscreenMode << " " << getResolutions().size();
  VideoMode vMode = getResolutions()[fullscreenMode];
  CHECK(vMode.isValid()) << "Video mode invalid: " << int(vMode.width) << " " << int(vMode.height) << " " <<
      int(vMode.bitsPerPixel) << " " << fullscreen;
  if (fullscreen)
    display.create(vMode, "KeeperRL", sf::Style::Fullscreen);
  else
    display.create(sf::VideoMode::getDesktopMode(), "KeeperRL");
  updateResolution();
  display.setVerticalSyncEnabled(true);
}

vector<string> Renderer::getFullscreenResolutions() {
  return transform2<string>(getResolutions(),
      [] (const VideoMode& m) { return toString(m.width) + "x" + toString(m.height);});
}

void Renderer::printSystemInfo(ostream& out) {
  sf::ContextSettings settings = display.getSettings();
  out << "OpenGL Major: " << settings.majorVersion << " minor: " << settings.minorVersion << std::endl;
}

Renderer::Renderer(const string& title, Vec2 nominal, const string& fontPath) : nominalSize(nominal) {
  CHECK(fonts.textFont.loadFromFile(fontPath + "/Lato-Bol.ttf"));
  CHECK(fonts.tileFont.loadFromFile(fontPath + "/Lato-Bol.ttf"));
  CHECK(fonts.symbolFont.loadFromFile(fontPath + "/Symbola.ttf"));
  fontsOtherThread = fonts;
  colors[ColorId::WHITE] = Color(255, 255, 255);
  colors[ColorId::YELLOW] = Color(250, 255, 0);
  colors[ColorId::LIGHT_BROWN] = Color(210, 150, 0);
  colors[ColorId::ORANGE_BROWN] = Color(250, 150, 0);
  colors[ColorId::BROWN] = Color(240, 130, 0);
  colors[ColorId::DARK_BROWN] = Color(100, 60, 0);
  colors[ColorId::MAIN_MENU_ON] = Color(255, 255, 255, 190);
  colors[ColorId::MAIN_MENU_OFF] = Color(255, 255, 255, 70);
  colors[ColorId::LIGHT_GRAY] = Color(150, 150, 150);
  colors[ColorId::GRAY] = Color(100, 100, 100);
  colors[ColorId::ALMOST_GRAY] = Color(102, 102, 102);
  colors[ColorId::DARK_GRAY] = Color(50, 50, 50);
  colors[ColorId::ALMOST_BLACK] = Color(20, 20, 20);
  colors[ColorId::ALMOST_DARK_GRAY] = Color(60, 60, 60);
  colors[ColorId::BLACK] = Color(0, 0, 0);
  colors[ColorId::ALMOST_WHITE] = Color(200, 200, 200);
  colors[ColorId::GREEN] = Color(0, 255, 0);
  colors[ColorId::LIGHT_GREEN] = Color(100, 255, 100);
  colors[ColorId::DARK_GREEN] = Color(0, 150, 0);
  colors[ColorId::RED] = Color(255, 0, 0);
  colors[ColorId::LIGHT_RED] = Color(255, 100, 100);
  colors[ColorId::PINK] = Color(255, 20, 147);
  colors[ColorId::ORANGE] = Color(255, 165, 0);
  colors[ColorId::BLUE] = Color(0, 0, 255);
  colors[ColorId::NIGHT_BLUE] = Color(0, 0, 20);
  colors[ColorId::DARK_BLUE] = Color(50, 50, 200);
  colors[ColorId::LIGHT_BLUE] = Color(100, 100, 255);
  colors[ColorId::PURPLE] = Color(160, 32, 240);
  colors[ColorId::VIOLET] = Color(120, 0, 255);
  colors[ColorId::TRANSLUCENT_BLACK] = Color(0, 0, 0);
  colors[ColorId::TRANSPARENT] = Color(0, 0, 0, 0);
}

Vec2 getOffset(Vec2 sizeDiff, double scale) {
  return Vec2(round(sizeDiff.x * scale * 0.5), round(sizeDiff.y * scale * 0.5));
}

Color Renderer::getBleedingColor(const ViewObject& object) {
  double bleeding = object.getAttribute(ViewObject::Attribute::BLEEDING);
  if (bleeding > 0)
    bleeding = 0.3 + bleeding * 0.7;
  return Color(255, max(0., (1 - bleeding) * 255), max(0., (1 - bleeding) * 255));
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
  drawSprite(pos + off, coord.pos.mult(sz), sz, tiles.at(coord.texNum), sz * scale, color);
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, bool useSprite, double scale, Color color) {
  const Tile& tile = Tile::getTile(id, useSprite);
  if (tile.hasSpriteCoord())
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), scale, color * tile.color);
  else
    drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 20 * scale, tile.color, pos.x, pos.y,
        tile.text);
}

void Renderer::drawViewObject(Vec2 pos, ViewId id, bool useSprite, Vec2 size, Color color) {
  const Tile& tile = Tile::getTile(id, useSprite);
  if (tile.hasSpriteCoord())
    drawTile(pos, tile.getSpriteCoord(DirSet::fullSet()), size, color * tile.color);
  else
    drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, size.y, tile.color, pos.x, pos.y, tile.text);
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object, bool useSprite, Vec2 size) {
  drawViewObject(pos, object.id(), useSprite, size, getBleedingColor(object));
}

void Renderer::drawViewObject(Vec2 pos, const ViewObject& object, bool useSprite, double scale) {
  drawViewObject(pos, object.id(), useSprite, scale, getBleedingColor(object));
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
  Image image;
  image.create(setWidth, ((files.size() + rowLength - 1) / rowLength) * size.y);
  for (int i : All(files)) {
    Image im;
    CHECK(im.loadFromFile((path + "/" + files[i]).c_str())) << "Failed to load " << files[i];
    CHECK(im.getSize().x == size.x && im.getSize().y == size.y)
        << files[i] << " has wrong size " << int(im.getSize().x) << " " << int(im.getSize().y);
    image.copy(im, size.x * (i % rowLength), size.y * (i / rowLength));
    CHECK(!tileCoords.count(files[i])) << "Duplicate name " << files[i];
    tileCoords[files[i].substr(0, files[i].size() - imageSuf.size())] =
        {{i % rowLength, i / rowLength}, int(tiles.size())};
  }
  tiles.emplace_back();
  tiles.back().loadFromImage(image);
  return true;
}

Renderer::TileCoord Renderer::getTileCoord(const string& name) {
  CHECK(tileCoords.count(name)) << "Tile not found " << name;
  return tileCoords.at(name);
}

Vec2 Renderer::getNominalSize() const {
  return nominalSize;
}

bool Renderer::loadTilesFromFile(const string& path, Vec2 size) {
  tileSize.push_back(size);
  tiles.emplace_back();
  return tiles.back().loadFromFile(path.c_str());
}

void Renderer::drawAndClearBuffer() {
  for (int i : All(renderList)) {
    for (auto& elem : renderList[i])
      elem();
    renderList[i].clear();
  }
  display.display();
  display.clear(Color(0, 0, 0));
}

void Renderer::resize(int width, int height) {
  updateResolution();
}

Event Renderer::getRandomEvent() {
  Event::EventType type = Event::EventType(Random.get(int(Event::Count)));
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
  return ret;
}

bool Renderer::pollEventWorkaroundMouseReleaseBug(Event& ev) {
  if (genReleaseEvent && !Mouse::isButtonPressed(Mouse::Right) && !Mouse::isButtonPressed(Mouse::Left)) {
    ev.type = Event::MouseButtonReleased;
    ev.mouseButton = {Mouse::Left, Mouse::getPosition(display).x / zoom, Mouse::getPosition(display).y / zoom};
    genReleaseEvent = false;
    return true;
  }
  bool was = pollEventOrFromQueue(ev);
  if (!was)
    return false;
  if (ev.type == Event::MouseButtonPressed) {
    genReleaseEvent = true;
    return true;
  } else if (ev.type == Event::MouseButtonReleased)
    return false;
  else
    return true;
}

bool Renderer::pollEventOrFromQueue(Event& ev) {
  if (!eventQueue.empty()) {
    ev = eventQueue.front();
    eventQueue.pop_front();
    return true;
  } else if (display.pollEvent(ev)) {
    zoomMousePos(ev);
    considerMouseMoveEvent(ev);
    return true;
  } else
    return false;
}

void Renderer::considerMouseMoveEvent(Event& ev) {
  if (ev.type == Event::MouseMoved)
    mousePos = Vec2(ev.mouseMove.x, ev.mouseMove.y);
}

bool Renderer::pollEvent(Event& ev) {
  CHECK(currentThreadId() == *renderThreadId);
  if (monkey) {
    if (Random.roll(2))
      return false;
    ev = getRandomEvent();
    return true;
  } else 
    return pollEventWorkaroundMouseReleaseBug(ev);
}

void Renderer::flushEvents(Event::EventType type) {
  Event ev;
  while (display.pollEvent(ev)) {
    zoomMousePos(ev);
    considerMouseMoveEvent(ev);
    if (ev.type != type)
      eventQueue.push_back(ev);
  }
}

void Renderer::zoomMousePos(Event& ev) {
  switch (ev.type) {
    case Event::MouseButtonReleased:
    case Event::MouseButtonPressed:
      ev.mouseButton.x /= zoom;
      ev.mouseButton.y /= zoom;
      break;
    case Event::MouseMoved:
      ev.mouseMove.x /= zoom;
      ev.mouseMove.y /= zoom;
      break;
    case Event::MouseWheelMoved:
      ev.mouseWheel.x /= zoom;
      ev.mouseWheel.y /= zoom;
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
      display.waitEvent(ev);
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
