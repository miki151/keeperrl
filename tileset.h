#pragma once

#include "util.h"
#include "view_id.h"
#include "tile.h"

class Renderer;
class GameConfig;
class Tile;
struct Color;

class TileSet {
  public:
  TileSet(Renderer&, GameConfig* config, bool useTiles);
  const Tile& getTile(ViewId id, bool sprite = true) const;
  Color getColor(const ViewObject&) const;

  private:
  friend class TileCoordLookup;
  void addTile(ViewId, Tile);
  void addSymbol(ViewId, Tile);
  unordered_map<ViewId, Tile, CustomHash<ViewId>> tiles;
  unordered_map<ViewId, Tile, CustomHash<ViewId>> symbols;
};
