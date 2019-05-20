#pragma once

#include "util.h"
#include "view_id.h"
#include "tile.h"
#include "tile_paths.h"

class Renderer;
class GameConfig;
class Tile;
struct Color;

struct TileCoord {
  Vec2 size;
  Vec2 pos;
  Texture* texture;
};

class TileSet {
  public:
  TileSet(const DirectoryPath& defaultDir, const DirectoryPath& modsDir);
  void setTilePaths(const TilePaths&);
  const TilePaths& getTilePaths() const;
  void reload(bool useTiles);
  const Tile& getTile(ViewId id, bool sprite = true) const;
  Color getColor(const ViewObject&) const;
  const vector<TileCoord>& getTileCoord(const string&) const;

  private:
  optional<TilePaths> tilePaths;
  DirectoryPath defaultDir;
  DirectoryPath modsDir;
  friend class TileCoordLookup;
  void addTile(string, Tile);
  void addSymbol(string, Tile);
  unordered_map<ViewId::InternalId, Tile> tiles;
  unordered_map<ViewId::InternalId, Tile> symbols;
  vector<unique_ptr<Texture>> textures;
  map<string, vector<TileCoord>> tileCoords;
  void loadTilesFromDir(const DirectoryPath&, Vec2 size);
  void loadTiles();
  void loadUnicode();
  const vector<TileCoord>& byName(const string&);
  Tile sprite(const string&);
  Tile empty();
  Tile getRoadTile(const string& prefix);
  Tile getWallTile(const string& prefix);
  Tile getMountainTile(const string& spriteName, const string& prefix);
  Tile getWaterTile(const string& background, const string& prefix);
  Tile getExtraBorderTile(const string& prefix);
  Tile symbol(const string& s, Color id, bool symbol = false);
  void loadModdedTiles(const vector<TileInfo>& tiles, bool useTiles);
};
