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

Color transparency(const Color& color, int trans) {
  return Color(color.r, color.g, color.b, trans);
}

vector<Texture> Renderer::tiles;
vector<Vec2> Renderer::tileSize;
Vec2 Renderer::nominalSize;
map<string, Renderer::TileCoords> Renderer::tileCoords;

int Renderer::getTextLength(string s) {
  Text t(s, textFont, textSize);
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
  display->draw(t);
}

void Renderer::drawText(Color color, int x, int y, string s, bool center, int size) {
  std::basic_string<sf::Uint32> utf32;
  sf::Utf8::toUtf32(s.begin(), s.end(), std::back_inserter(utf32));
  drawText(TEXT_FONT, size, color, x, y, utf32, center);
}

void Renderer::drawText(Color color, int x, int y, const char* c, bool center, int size) {
  drawText(TEXT_FONT, size, color, x, y, String(c), center);
}

void Renderer::drawTextWithHotkey(Color color, int x, int y, const string& text, char key) {
  if (key) {
    int ind = lowercase(text).find(key);
    if (ind != string::npos) {
      int pos = x + getTextLength(text.substr(0, ind));
      drawFilledRectangle(pos, y + 23, pos + getTextLength(text.substr(ind, 1)), y + 25, colors[ColorId::GRAY]);
    }
  }
  drawText(color, x, y, text);
}

void Renderer::drawImage(int px, int py, const Image& image, double scale) {
  drawImage(px, py, px + image.getSize().x * scale, py + image.getSize().y * scale, image, scale);
}

void Renderer::drawImage(Rectangle r, const Image& image, double scale) {
  drawImage(r.getPX(), r.getPY(), r.getKX(), r.getKY(), image, scale);
}

void Renderer::drawImage(int px, int py, int kx, int ky, const Image& image, double scale) {
  Texture t;
  t.loadFromImage(image);
  Sprite s(t, sf::IntRect(0, 0, (kx - px) / scale, (ky - py) / scale));
  s.setPosition(px, py);
  if (scale != 1)
    s.setScale(scale, scale);
  display->draw(s);
}

void Renderer::drawSprite(Vec2 pos, Vec2 spos, Vec2 size, const Texture& t, Optional<Color> color) {
  drawSprite(pos.x, pos.y, spos.x, spos.y, size.x, size.y, t, -1, -1, color);
}

void Renderer::drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t, int dw, int dh,
    Optional<Color> color) {
  Sprite s(t, sf::IntRect(px, py, w, h));
  s.setPosition(x, y);
  if (color)
    s.setColor(*color);
  if (dw != -1)
    s.setScale(double(dw) / w, double(dh) / h);
  display->draw(s);
}

void Renderer::drawFilledRectangle(const Rectangle& t, Color color, Optional<Color> outline) {
  RectangleShape r(Vector2f(t.getW(), t.getH()));
  r.setPosition(t.getPX(), t.getPY());
  r.setFillColor(color);
  if (outline) {
    r.setOutlineThickness(-2);
    r.setOutlineColor(*outline);
  }
  display->draw(r);
}

void Renderer::drawFilledRectangle(int px, int py, int kx, int ky, Color color, Optional<Color> outline) {
  drawFilledRectangle(Rectangle(px, py, kx, ky), color, outline);
}

int Renderer::getWidth() {
  return display->getSize().x;
}

int Renderer::getHeight() {
  return display->getSize().y;
}

void Renderer::initialize(RenderTarget* d, int width, int height) {
  display = d;
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

}

void Renderer::drawViewObject(int x, int y, const ViewObject& object, bool useSprite, double scale) {
  Tile tile = Tile::getTile(object, useSprite);
  if (tile.hasSpriteCoord()) {
    Vec2 sz = Renderer::tileSize[tile.getTexNum()];
    Vec2 of = (Renderer::nominalSize - sz) / 2;
    Vec2 coord = tile.getSpriteCoord(EnumSet<Dir>::fullSet());
    drawSprite(x + of.x, y + of.y, coord.x * sz.x, coord.y * sz.y, sz.x, sz.y, Renderer::tiles[tile.getTexNum()],
        sz.x * scale, sz.y * scale);
  } else
    drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 20 * scale,
        Tile::getColor(object), x, y, tile.text);
}

const static string imageSuf = ".png";

bool Renderer::loadTilesFromDir(const string& path, Vec2 size) {
  tileSize.push_back(size);
  struct dirent *ent;
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

Renderer::TileCoords Renderer::getTileCoords(const string& name) {
  CHECK(tileCoords.count(name)) << "Tile not found " << name;
  return tileCoords.at(name);
}

bool Renderer::loadTilesFromFile(const string& path, Vec2 size) {
  tileSize.push_back(size);
  tiles.emplace_back();
  return tiles.back().loadFromFile(path.c_str());
}

void Renderer::setNominalSize(Vec2 sz) {
  nominalSize = sz;
}

