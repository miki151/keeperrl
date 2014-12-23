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
#include "script_context.h"
#include "gui_elem.h"

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
  String tmp = Renderer::toUnicode(ch);
  CHECK(tmp.getSize() == 1) << "Symbol text too long: " << ch;
  return unicode(tmp[0], colorId, symbol);
}

Tile::Tile(TileCoord c) : tileCoord(c) {
}

Tile Tile::setNoShadow() {
  noShadow = true;
  return *this;
}

Tile Tile::byCoord(TileCoord c) {
  return Tile(c);
}

Tile Tile::setFloorBorders() {
  floorBorders = true;
  return *this;
}

Tile Tile::addConnection(EnumSet<Dir> c, TileCoord coord) {
  connections.insert({c, coord});
  return *this;
}

Tile Tile::addOption(Dir d, TileCoord coord) {
  connectionOption = {d, coord};
  return *this;
}

Tile Tile::addBackground(TileCoord coord) {
  backgroundCoord = coord;
  return *this;
}

Tile Tile::addExtraBorder(EnumSet<Dir> dir, TileCoord coord) {
  extraBorders[dir] = coord;
  return *this;
}

Tile Tile::addExtraBorderId(ViewId id) {
  extraBorderIds.insert(id);
  return *this;
}

const EnumSet<ViewId>& Tile::getExtraBorderIds() const {
  return extraBorderIds;
}

bool Tile::hasExtraBorders() const {
  return !extraBorders.empty();
}

Optional<Tile::TileCoord> Tile::getExtraBorderCoord(const EnumSet<Dir>& c) const {
  if (extraBorders.count(c))
    return extraBorders.at(c);
  else
    return Nothing();
}

Tile Tile::setTranslucent(double v) {
  translucent = v;
  return *this;
}

bool Tile::hasSpriteCoord() const {
  return tileCoord;
}

Tile::TileCoord Tile::getSpriteCoord() const {
  return *tileCoord;
}

Optional<Tile::TileCoord> Tile::getBackgroundCoord() const {
  return backgroundCoord;
}

Tile::TileCoord Tile::getSpriteCoord(const EnumSet<Dir>& c) const {
  if (connectionOption) {
    CHECK(connections.empty()) << "Can't have connections and options at the same time";
    if (c[connectionOption->first])
      return connectionOption->second;
    else
      return *tileCoord;
  }
  if (connections.count(c))
    return connections.at(c);
  else return *tileCoord;
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

static SetOfDir dirs() {
  return SetOfDir();
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

static void setBackground(int r, int g, int b) {
  GuiElem::setBackground(r, g, b);
}

}

class TileCoordLookup {
  public:
  TileCoordLookup(Renderer& r) : renderer(r) {}

  Tile::TileCoord byName(const string& s) {
    return renderer.getTileCoord(s);
  }

  Tile::TileCoord getCoords(int x, int y, int tex) {
    return {Vec2(x, y), tex};
  }

  private:
  Renderer& renderer;
};

void Tile::initialize(Renderer& renderer, bool useTiles) {
  TileCoordLookup lookup(renderer);
  ADD_SCRIPT_VALUE_TYPE(Tile);
  ADD_SCRIPT_VALUE_TYPE(TileCoord);
  ADD_SCRIPT_ENUM(Dir);
  ADD_SCRIPT_ENUM(ColorId);
  ADD_SCRIPT_VALUE_TYPE(SetOfDir);
  ADD_SCRIPT_ENUM(ViewId);
  ADD_SCRIPT_FUNCTION(setBackground, "void setGuiBackground(int, int, int)");
  ADD_SCRIPT_FUNCTION(Tile::byCoord, "Tile sprite(TileCoord)");
  ADD_SCRIPT_FUNCTION_FROM_SINGLETON(TileCoordLookup::byName, "TileCoord byName(const string& in)",
      lookup);
  ADD_SCRIPT_FUNCTION_FROM_SINGLETON(TileCoordLookup::getCoords, "TileCoord byCoord(int x, int y, int tex)",
      lookup);
  ADD_SCRIPT_FUNCTION(Tile::fromString, "Tile symbol(const string& in, ColorId, bool = false)");
  ADD_SCRIPT_FUNCTION(Tile::unicode, "Tile symbol(uint, ColorId, bool = false)");
  ADD_SCRIPT_FUNCTION(addTile, "void addTile(ViewId, Tile)");
  ADD_SCRIPT_FUNCTION(addSymbol, "void addSymbol(ViewId, Tile)");
  ADD_SCRIPT_FUNCTION(SetOfDir::fullSet, "SetOfDir setOfAllDirs()");
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs()", (), SetOfDir);
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir)", (Dir), SetOfDir);
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir, Dir)", (Dir, Dir), SetOfDir);
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir, Dir, Dir)", (Dir, Dir, Dir), SetOfDir);
  ADD_SCRIPT_FUNCTION_OVERLOAD(dirs, "SetOfDir dirs(Dir, Dir, Dir, Dir)", (Dir, Dir, Dir, Dir), SetOfDir);
  ADD_SCRIPT_METHOD(Tile, setNoShadow, "Tile setNoShadow()");
  ADD_SCRIPT_METHOD(Tile, setTranslucent, "Tile setTranslucent(double)");
  ADD_SCRIPT_METHOD(Tile, setFloorBorders, "Tile setFloorBorders()");
  ADD_SCRIPT_METHOD_OVERLOAD(Tile, addBackground, "Tile addBackground(TileCoord)", (TileCoord), Tile);
  ADD_SCRIPT_METHOD_OVERLOAD(Tile, addConnection, "Tile addConnection(SetOfDir c, TileCoord)",
      (SetOfDir, TileCoord), Tile);
  ADD_SCRIPT_METHOD_OVERLOAD(Tile, addOption, "Tile addOption(Dir, TileCoord)",
      (Dir, TileCoord), Tile);
  ADD_SCRIPT_METHOD(Tile, addCorner, "Tile addCorner(SetOfDir def, SetOfDir borders, TileCoord)");
  ADD_SCRIPT_METHOD(Tile, addExtraBorder, "Tile addExtraBorder(SetOfDir def, TileCoord)");
  ADD_SCRIPT_METHOD(Tile, addExtraBorderId, "Tile addExtraBorderId(ViewId)");
  loadUnicode();
  if (useTiles)
    loadTiles();
}

void Tile::loadTiles() {
  ScriptContext::execute("tiles.as", "void genTiles()");
  bool bad = false;
  for (ViewId id : ENUM_ALL(ViewId))
    if (!tiles.count(id)) {
      Debug() << "ViewId not found: " << EnumInfo<ViewId>::getString(id);
      bad = true;
    }
  CHECK(!bad);
}

void Tile::loadUnicode() {
  ScriptContext::execute("tiles.as", "void genSymbols()");
  bool bad = false;
  for (ViewId id : ENUM_ALL(ViewId))
    if (!symbols.count(id)) {
      Debug() << "ViewId not found: " << EnumInfo<ViewId>::getString(id);
      bad = true;
    }
  CHECK(!bad);
}

const Tile& Tile::fromViewId(ViewId id) {
  CHECK(tiles.count(id));
  return tiles.at(id);
}

const Tile& getSpriteTile(ViewId id) {
  if (!tiles.count(id))
    FAIL << "unhandled view id " << EnumInfo<ViewId>::getString(id);
  return tiles.at(id);
}

const Tile& getAsciiTile(ViewId id) {
  if (!symbols.count(id))
    FAIL << "unhandled view id " << EnumInfo<ViewId>::getString(id);
  return symbols.at(id);
}

const Tile& Tile::getTile(ViewId id, bool sprite) {
  if (sprite)
    return getSpriteTile(id);
  else
    return getAsciiTile(id);
}

Color Tile::getColor(const ViewObject& object) {
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE))
    return colors[ColorId::DARK_GRAY];
  if (object.hasModifier(ViewObject::Modifier::HIDDEN))
    return colors[ColorId::LIGHT_GRAY];
  Color color = getAsciiTile(object.id()).color;
  if (object.hasModifier(ViewObject::Modifier::PLANNED)) {
    return Color((color.r) / 2, (color.g) / 2, (color.b) / 2);
  }
  double bleeding = object.getAttribute(ViewObject::Attribute::BLEEDING);
  if (bleeding > 0) {
    bleeding = 0.5 + bleeding / 2;
    bleeding = min(1., bleeding);
    return Color(
        (1 - bleeding) * color.r + bleeding * 255,
        (1 - bleeding) * color.g,
        (1 - bleeding) * color.b);
  } else
    return color;
}

Tile Tile::addCorner(EnumSet<Dir> cornerDef, EnumSet<Dir> borders, TileCoord coord) {
  corners.push_back({cornerDef, borders, coord});
  return *this;
}

bool Tile::hasCorners() const {
  return !corners.empty();
}

vector<Tile::TileCoord> Tile::getCornerCoords(const EnumSet<Dir>& c) const {
  vector<TileCoord> ret;
  for (auto& elem : corners) {
    if (elem.cornerDef.intersection(c) == elem.borders)
      ret.push_back(elem.tileCoord);
  }
  return ret;
}

