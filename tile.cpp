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
#include "gui_elem.h"
#include "view_id.h"
#include "view_object.h"
#include "tile_info.h"
#include "game_config.h"
#include "tileset.h"

Tile::Tile() {
}

Tile Tile::fromString(const string& ch, Color color, bool symbol) {
  Tile ret;
  ret.color = color;
  ret.text = ch;
  ret.symFont = symbol;
  return ret;
}

Tile::Tile(const vector<TileCoord>& c) : color{255, 255, 255}, tileCoord(c) {
}

Tile Tile::setRoundShadow() {
  roundShadow = true;
  moveUp = true;
  return *this;
}

Tile Tile::setMoveUp() {
  moveUp = true;
  return *this;
}

Tile Tile::setWallShadow() {
  wallShadow = true;
  return *this;
}

Tile Tile::setFX(FXInfo f) {
  fx = f;
  return *this;
}

Tile Tile::setConnectionId(ViewId id) {
  connectionId = id;
  return *this;
}

Tile Tile::byCoord(const vector<TileCoord>& c) {
  return Tile(c);
}

Tile Tile::setFloorBorders() {
  floorBorders = true;
  return *this;
}

Tile Tile::addConnection(DirSet dirs, vector<TileCoord> coord) {
  connections[dirs] = coord;
  if (dirs & (~connectionsMask))
    connectionsMask = DirSet::fullSet();
  anyConnections = true;
  return *this;
}

Tile Tile::addOption(Dir d, vector<TileCoord> coord) {
  connectionOption = make_pair(d, coord);
  anyConnections = true;
  return *this;
}

Tile Tile::addBackground(vector<TileCoord> coord) {
  backgroundCoord = coord;
  return *this;
}

Tile Tile::setColor(Color col) {
  color = col;
  return *this;
}

Tile Tile::addExtraBorder(DirSet dir, const vector<TileCoord>& coord) {
  extraBorders[dir] = coord;
  return *this;
}

Tile Tile::addExtraBorderId(ViewId id) {
  extraBorderIds.insert(id);
  anyExtraBorders = true;
  return *this;
}

const unordered_set<ViewId, CustomHash<ViewId>>& Tile::getExtraBorderIds() const {
  return extraBorderIds;
}

bool Tile::hasExtraBorders() const {
  return anyExtraBorders;
}

const vector<TileCoord>& Tile::getExtraBorderCoord(DirSet c) const {
  return extraBorders[c];
}

const optional<ViewId>&Tile::getConnectionId() const {
  return connectionId;
}

Tile Tile::setTranslucent(double v) {
  translucent = v;
  return *this;
}

bool Tile::hasSpriteCoord() const {
  return !tileCoord.empty();
}

const vector<TileCoord>& Tile::getSpriteCoord() const {
  CHECK(!tileCoord.empty());
  return tileCoord;
}

const vector<TileCoord>& Tile::getBackgroundCoord() const {
  return backgroundCoord;
}

bool Tile::hasAnyConnections() const {
  return anyConnections;
}

const vector<TileCoord>& Tile::getSpriteCoord(DirSet c) const {
  if (connectionOption) {
    if (c.has(connectionOption->first))
      return connectionOption->second;
    else {
      CHECK(!tileCoord.empty());
      return tileCoord;
    }
  }
  c = c & connectionsMask;
  if (!connections[c].empty())
    return connections[c];
  else {
    CHECK(!tileCoord.empty());
    return tileCoord;
  }
}

Tile Tile::addCorner(DirSet corner, DirSet borders, vector<TileCoord> coord) {
  anyCorners = true;
  for (DirSet dirs : Range(0, 255))
    if ((dirs & corner) == borders)
      corners[dirs].push_back(coord.getOnlyElement());
  return *this;
}

bool Tile::hasAnyCorners() const {
  return anyCorners;
}

const vector<TileCoord>& Tile::getCornerCoords(DirSet c) const {
  return corners[c];
}

const optional<FXInfo>& Tile::getFX() const {
  return fx;
}

