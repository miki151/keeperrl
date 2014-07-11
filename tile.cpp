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
#include "tile.h"
#include "view_id.h"
#include "script_context.h"

Tile::Tile() {
}

Tile Tile::unicode(sf::Uint32 ch, ColorId col, bool sym) {
  Tile ret;
  ret.color = colors[col];
  ret.text = ch;
  ret.symFont = sym;
  return ret;
}

Tile Tile::fromString(const string& ch, ColorId colorId, bool symbol) {
  std::basic_string<sf::Uint32> tmp;
  sf::Utf8::toUtf32(ch.begin(), ch.end(), std::back_inserter(tmp));
  CHECK(tmp.size() == 1) << "Symbol text too long: " << ch;
  return Tile::unicode(tmp[0], colorId, symbol);
}

Tile::Tile(int x, int y, int num, bool _noShadow) : noShadow(_noShadow), tileCoord(Vec2(x, y)), texNum(num) {
}

Tile Tile::setNoShadow() {
  noShadow = true;
  return *this;
}

Tile Tile::byCoord(int x, int y, int num, bool noShadow) {
  return Tile(x, y, num, noShadow);
}

Tile Tile::byName(const string& s, bool noShadow) {
  return Tile(Renderer::getTileCoords(s), noShadow);
}

Tile Tile::empty() {
  return Tile::byCoord(-1, -1, -1);
}

Tile::Tile(Renderer::TileCoords coords, bool noShadow)
    : Tile(coords.pos.x, coords.pos.y, coords.texNum, noShadow) {}

Tile Tile::addConnection(EnumSet<Dir> c, int x, int y) {
  connections.insert({c, Vec2(x, y)});
  return *this;
}

Tile Tile::addConnection(EnumSet<Dir> c, const string& name) {
  Renderer::TileCoords coords = Renderer::getTileCoords(name);
  texNum = coords.texNum;
  return addConnection(c, coords.pos.x, coords.pos.y);
}

Tile Tile::addBackground(int x, int y) {
  backgroundCoord = Vec2(x, y);
  return *this;
}

Tile Tile::addBackground(const string& name) {
  Renderer::TileCoords coords = Renderer::getTileCoords(name);
  texNum = coords.texNum;
  return addBackground(coords.pos.x, coords.pos.y);
}

Tile Tile::setTranslucent(double v) {
  translucent = v;
  return *this;
}

bool Tile::hasSpriteCoord() {
  return tileCoord;
}

Vec2 Tile::getSpriteCoord() {
  return *tileCoord;
}

Optional<Vec2> Tile::getBackgroundCoord() {
  return backgroundCoord;
}

Vec2 Tile::getSpriteCoord(const EnumSet<Dir>& c) {
  if (connections.count(c))
    return connections.at(c);
  else return *tileCoord;
}

int Tile::getTexNum() {
  CHECK(tileCoord) << "Not a sprite tile";
  return texNum;
}

Tile getSpecialCreature(const ViewObject& obj, bool humanoid) {
  RandomGen r;
  r.init(hash<string>()(obj.getBareDescription()));
  string let = humanoid ? "WETUIPLKJHFAXBM" : "qwetyupkfaxbnm";
  char c;
  if (contains(let, obj.getBareDescription()[0]))
    c = obj.getBareDescription()[0];
  else
  if (contains(let, tolower(obj.getBareDescription()[0])))
    c = tolower(obj.getBareDescription()[0]);
  else
    c = let[r.getRandom(let.size())];
  return Tile::unicode(c, ColorId(Random.getRandom(EnumInfo<ColorId>::getSize())));
}

Tile getSpecialCreatureSprite(const ViewObject& obj, bool humanoid) {
  RandomGen r;
  r.init(hash<string>()(obj.getBareDescription()));
  if (humanoid)
    return Tile::byCoord(r.getRandom(7), 10);
  else
    return Tile::byCoord(r.getRandom(7, 10), 10);
}

static map<ViewId, Tile> tiles;
static map<ViewId, Tile> symbols;

typedef EnumSet<Dir> SetOfDir;

namespace {

void addTile(ViewId id, Tile tile) {
  tiles.erase(id);
  tiles.insert({id, tile});
}

void addSymbol(ViewId id, Tile tile) {
  symbols.erase(id);
  symbols.insert({id, tile});
}

static SetOfDir dirs(Dir d1) {
  return SetOfDir({d1});
}

static SetOfDir dirs(Dir d1, Dir d2) {
  return SetOfDir({d1, d2});
}

static SetOfDir dirs(Dir d1, Dir d2, Dir d3) {
  return SetOfDir({d1, d2, d3});
}

static SetOfDir dirs(Dir d1, Dir d2, Dir d3, Dir d4) {
  return SetOfDir({d1, d2, d3, d4});
}

}

void Tile::initialize() {
  ADD_SCRIPT_VALUE_TYPE(Tile);
  ADD_SCRIPT_ENUM(Dir);
  ADD_SCRIPT_ENUM(ColorId);
  ADD_SCRIPT_VALUE_TYPE(SetOfDir);
  ADD_SCRIPT_ENUM(ViewId);
  ADD_SCRIPT_FUNCTION(Tile::byCoord, "Tile sprite(int, int, int = 0, bool = false)");
  ADD_SCRIPT_FUNCTION(Tile::byName, "Tile sprite(const string& in, bool = false)");
  ADD_SCRIPT_FUNCTION(Tile::fromString, "Tile symbol(const string& in, ColorId, bool = false)");
  ADD_SCRIPT_FUNCTION(Tile::unicode, "Tile symbol(uint, ColorId, bool = false)");
  ADD_SCRIPT_FUNCTION(Tile::empty, "Tile empty()");
  ADD_SCRIPT_FUNCTION(addTile, "void addTile(ViewId, Tile)");
  ADD_SCRIPT_FUNCTION(addSymbol, "void addSymbol(ViewId, Tile)");
  ADD_SCRIPT_FUNCTION(SetOfDir::fullSet, "SetOfDir setOfAllDirs()");
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir)", (Dir), SetOfDir);
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir, Dir)", (Dir, Dir), SetOfDir);
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir, Dir, Dir)", (Dir, Dir, Dir), SetOfDir);
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir, Dir, Dir, Dir)", (Dir, Dir, Dir, Dir), SetOfDir);
  ADD_SCRIPT_METHOD(Tile, setNoShadow, "Tile setNoShadow()");
  ADD_SCRIPT_METHOD(Tile, setTranslucent, "Tile setTranslucent(double)");
  ADD_SCRIPT_METHOD_OVERLOAD(Tile, addBackground, "Tile addBackground(int, int)", (int, int), Tile);
  ADD_SCRIPT_METHOD_OVERLOAD(Tile, addBackground, "Tile addBackground(const string& in)", (const string&), Tile);
  ADD_SCRIPT_METHOD_OVERLOAD(Tile, addConnection, "Tile addConnection(SetOfDir c, int, int)",
      (SetOfDir, int, int), Tile);
  ADD_SCRIPT_METHOD_OVERLOAD(Tile, addConnection, "Tile addConnection(SetOfDir c, const string& in)",
      (SetOfDir, const string&), Tile);
  ScriptContext::execute("tiles.as", "void genTiles()");
  ScriptContext::execute("tiles.as", "void genSymbols()");
}

Tile getSpriteTile(const ViewObject& obj) {
  ViewId id = obj.id();
  if (id == ViewId::SPECIAL_BEAST)
    return getSpecialCreatureSprite(obj, false);
  if (id == ViewId::SPECIAL_HUMANOID)
    return getSpecialCreatureSprite(obj, true);
  if (tiles.count(id))
    return tiles.at(id);
  else
    FAIL << "unhandled view id " << EnumInfo<ViewId>::getString(id);
  return Tile::unicode(' ', ColorId(0));
}

Tile getAsciiTile(const ViewObject& obj) {
  ViewId id = obj.id();
  if (id == ViewId::SPECIAL_BEAST)
    return getSpecialCreature(obj, false);
  if (id == ViewId::SPECIAL_HUMANOID)
    return getSpecialCreature(obj, true);
  if (symbols.count(id))
    return symbols.at(id);
  else
    FAIL << "unhandled view id " << EnumInfo<ViewId>::getString(id);
  return Tile::unicode(' ', ColorId(0));
}

Tile Tile::getTile(const ViewObject& obj, bool sprite) {
  if (sprite)
    return getSpriteTile(obj);
  else
    return getAsciiTile(obj);
}

Color Tile::getColor(const ViewObject& object) {
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE))
    return colors[ColorId::DARK_GRAY];
  if (object.hasModifier(ViewObject::Modifier::HIDDEN))
    return colors[ColorId::LIGHT_GRAY];
  double bleeding = object.getAttribute(ViewObject::Attribute::BLEEDING);
  if (bleeding > 0)
    bleeding = 0.5 + bleeding / 2;
  bleeding = min(1., bleeding);
  Color color = getAsciiTile(object).color;
  return Color(
      (1 - bleeding) * color.r + bleeding * 255,
      (1 - bleeding) * color.g,
      (1 - bleeding) * color.b);
}


