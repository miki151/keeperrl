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

Font textFont;
Font tileFont;
Font symbolFont;

EnumMap<ColorId, Color> colors;

Renderer::TileCoord::TileCoord(Vec2 p, int t) : pos(p), texNum(t) {
}

Renderer::TileCoord::TileCoord() : TileCoord(Vec2(0, 0), -1) {
}

Color transparency(const Color& color, int trans) {
  return Color(color.r, color.g, color.b, trans);
}

int Renderer::getTextLength(string s) {
  Text t(toUnicode(s), textFont, textSize);
  return t.getLocalBounds().width;
}

Font& getFont(Renderer::FontId id) {
  switch (id) {
    case Renderer::TEXT_FONT: return textFont;
    case Renderer::TILE_FONT: return tileFont;
    case Renderer::SYMBOL_FONT: return symbolFont;
  }
  FAIL << "ewf";
  return textFont;
}

void Renderer::drawText(FontId id, int size, Color color, int x, int y, String s, bool center) {
  int ox = 0;
  int oy = 0;
  Text t(s, getFont(id), size);
  if (center) {
    sf::FloatRect bounds = t.getLocalBounds();
    ox -= bounds.left + bounds.width / 2;
    //oy -= bounds.top + bounds.height / 2;
  }
  t.setPosition(x + ox, y + oy);
  t.setColor(color);
  renderList.push_back(
      [this, t] { display->draw(t); });
}

String Renderer::toUnicode(const string& s) {
  std::basic_string<sf::Uint32> utf32;
  sf::Utf8::toUtf32(s.begin(), s.end(), std::back_inserter(utf32));
  return utf32;
}

void Renderer::drawText(Color color, int x, int y, string s, bool center, int size) {
  drawText(TEXT_FONT, size, color, x, y, toUnicode(s), center);
}

void Renderer::drawText(Color color, int x, int y, const char* c, bool center, int size) {
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
  drawSprite(target.getPX(), target.getPY(), source.getPX(), source.getPY(), source.getW(), source.getH(),
      image, target.getW(), target.getH());
}

void Renderer::drawImage(int px, int py, int kx, int ky, const Texture& t, double scale) {
  Sprite s(t, sf::IntRect(0, 0, (kx - px) / scale, (ky - py) / scale));
  s.setPosition(px, py);
  if (scale != 1) {
    s.setScale(scale, scale);
  }
  renderList.push_back(
      [this, s] { display->draw(s); });
}

void Renderer::drawSprite(Vec2 pos, Vec2 spos, Vec2 size, const Texture& t, optional<Color> color,
    optional<Vec2> stretchSize) {
  if (stretchSize)
    drawSprite(pos.x, pos.y, spos.x, spos.y, size.x, size.y, t, stretchSize->x, stretchSize->y, color);
  else
    drawSprite(pos.x, pos.y, spos.x, spos.y, size.x, size.y, t, -1, -1, color);
}

void Renderer::drawSprite(Vec2 pos, Vec2 stretchSize, const Texture& t) {
  drawSprite(pos.x, pos.y, 0, 0, t.getSize().x, t.getSize().y, t, stretchSize.x, stretchSize.y);
}

void Renderer::drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t, int dw, int dh,
    optional<Color> color) {
  Sprite s(t, sf::IntRect(px, py, w, h));
  s.setPosition(x, y);
  if (color)
    s.setColor(*color);
  if (dw != -1)
    s.setScale(double(dw) / w, double(dh) / h);
  renderList.push_back(
      [this, s] { display->draw(s); });
}

void Renderer::drawFilledRectangle(const Rectangle& t, Color color, optional<Color> outline) {
  RectangleShape r(Vector2f(t.getW(), t.getH()));
  r.setPosition(t.getPX(), t.getPY());
  r.setFillColor(color);
  if (outline) {
    r.setOutlineThickness(-2);
    r.setOutlineColor(*outline);
  }
  renderList.push_back(
      [this, r] { display->draw(r); });
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
  vector<Vertex>& quadsTmp = quads;
  renderList.push_back(
      [this, quadsTmp] { display->draw(&quadsTmp[0], quadsTmp.size(), sf::Quads); });
  quads.clear();
}

int Renderer::getWidth() {
  return display->getSize().x;
}

int Renderer::getHeight() {
  return display->getSize().y;
}

void Renderer::initialize() {
  display = new RenderWindow(sf::VideoMode::getDesktopMode(), "KeeperRL");
  sfView = new sf::View(display->getDefaultView());
}

Renderer::Renderer(const string& title, Vec2 nominal) : nominalSize(nominal) {
  CHECK(textFont.loadFromFile("Lato-Bol.ttf"));
  CHECK(tileFont.loadFromFile("Lato-Bol.ttf"));
  CHECK(symbolFont.loadFromFile("Symbola.ttf"));
  colors[ColorId::WHITE] = Color(255, 255, 255);
  colors[ColorId::YELLOW] = Color(250, 255, 0);
  colors[ColorId::LIGHT_BROWN] = Color(210, 150, 0);
  colors[ColorId::ORANGE_BROWN] = Color(250, 150, 0);
  colors[ColorId::BROWN] = Color(240, 130, 0);
  colors[ColorId::DARK_BROWN] = Color(100, 60, 0);
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

void Renderer::drawTile(int x, int y, TileCoord coord, int sizeX, int sizeY, Color color) {
  Vec2 sz = Renderer::tileSize[coord.texNum];
  Vec2 off = (nominalSize -  sz).mult(Vec2(sizeX, sizeY)).div(Renderer::nominalSize * 2);
  int width = sz.x * sizeX / nominalSize.x;
  int height = sz.y* sizeY / nominalSize.y;
  if (sz.y > nominalSize.y)
    off.y *= 2;
  CHECK(coord.texNum >= 0 && coord.texNum < Renderer::tiles.size());
  drawSprite(x + off.x, y + off.y, coord.pos.x * sz.x, coord.pos.y * sz.y, sz.x, sz.y,
      Renderer::tiles.at(coord.texNum), width, height, color);
}

void Renderer::drawTile(int x, int y, TileCoord coord, double scale, Color color) {
  CHECK(coord.texNum >= 0 && coord.texNum < Renderer::tiles.size());
  Vec2 sz = Renderer::tileSize[coord.texNum];
  Vec2 of = getOffset(Renderer::nominalSize - sz, scale);
  drawSprite(x + of.x, y + of.y, coord.pos.x * sz.x, coord.pos.y * sz.y, sz.x, sz.y,
      Renderer::tiles.at(coord.texNum), sz.x * scale, sz.y * scale, color);
}

void Renderer::drawViewObject(int x, int y, ViewId id, bool useSprite, double scale, Color color) {
  const Tile& tile = Tile::getTile(id, useSprite);
  if (tile.hasSpriteCoord())
    drawTile(x, y, tile.getSpriteCoord(EnumSet<Dir>::fullSet()), scale, color);
  else
    drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 20, tile.color, x, y, tile.text);
}

void Renderer::drawViewObject(int x, int y, const ViewObject& object, bool useSprite, double scale) {
  drawViewObject(x, y, object.id(), useSprite, scale, getBleedingColor(object));
}

const static string imageSuf = ".png";

bool Renderer::loadTilesFromDir(const string& path, Vec2 size) {
  tileSize.push_back(size);
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
  int imageWidth = 720;
  int rowLength = imageWidth / size.x;
  Image image;
  image.create(imageWidth, ((files.size() + rowLength - 1) / rowLength) * size.y);
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
  for (auto& elem : renderList)
    elem();
  renderList.clear();
  display->display();
  display->clear(Color(0, 0, 0));
}

void Renderer::resize(int width, int height) {
  display->setView(*(sfView = new sf::View(sf::FloatRect(0, 0, width, height))));
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
      ret.mouseButton = { chooseRandom({Mouse::Left, Mouse::Right}), Random.get(getWidth()),
        Random.get(getHeight()) };
      break;
    case Event::MouseMoved:
      ret.mouseMove = { Random.get(getWidth()), Random.get(getHeight()) };
      break;
    default: return getRandomEvent();
  }
  return ret;
}

bool Renderer::pollEventWorkaroundMouseReleaseBug(Event& ev) {
  if (genReleaseEvent && !Mouse::isButtonPressed(Mouse::Right) && !Mouse::isButtonPressed(Mouse::Left)) {
    ev.type = Event::MouseButtonReleased;
    ev.mouseButton = {Mouse::Left, Mouse::getPosition().x, Mouse::getPosition().y};
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
  } else
    return display->pollEvent(ev);
}

bool Renderer::pollEvent(Event& ev) {
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
  while (display->pollEvent(ev)) {
    if (ev.type != type)
      eventQueue.push_back(ev);
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
    } else
      display->waitEvent(ev);
  }
}

Vec2 Renderer::getMousePos() {
  if (monkey)
    return Vec2(Random.get(getWidth()), Random.get(getHeight()));
  auto pos = Mouse::getPosition(*display);
  return Vec2(pos.x, pos.y);
}

void Renderer::startMonkey() {
  monkey = true;
}
  
bool Renderer::isMonkey() {
  return monkey;
}
