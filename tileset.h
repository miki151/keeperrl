#pragma once

#include "util.h"
#include "view_id.h"
#include "tile.h"
#include "tile_paths.h"
#include "scripted_ui.h"

class Renderer;
class GameConfig;
class Tile;
struct Color;
class ScriptedUI;

struct TileCoord {
  Vec2 size;
  Vec2 pos;
  Texture* texture = nullptr;
};

class TileSet {
  public:
  TileSet(const DirectoryPath& defaultDir, const DirectoryPath& modsDir, const DirectoryPath& scriptedHelpDir);
  void setTilePaths(const TilePaths&);
  void setTilePathsAndReload(const TilePaths&);
  const TilePaths& getTilePaths() const;
  void reload();
  void clear();
  void loadTextures();
  const Tile& getTile(ViewId id, bool sprite = true) const;
  Color getColor(const ViewObject&) const;
  Color getColor(ViewId) const;
  const vector<TileCoord>& getTileCoord(const string&) const;
  const vector<string> getSpriteMods() const;

  map<string, Texture> scriptedUITextures;
  map<ScriptedUIId, ScriptedUI> scriptedUI;

  private:
  optional<TilePaths> tilePaths;
  DirectoryPath defaultDir;
  DirectoryPath modsDir;
  DirectoryPath scriptedHelpDir;
  friend class TileCoordLookup;
  void addTile(string, Tile);
  void addSymbol(string, Tile);
  unordered_map<ViewId::InternalId, Tile> tiles;
  unordered_map<ViewId::InternalId, Tile> symbols;
  vector<unique_ptr<Texture>> textures;
  struct TmpInfo {
    SDL::SDL_Surface* image;
    vector<pair<string, Vec2>> addedPositions;
  };
  vector<TmpInfo> texturesTmp;
  map<string, vector<TileCoord>> tileCoords;
  vector<string> spriteMods;
  bool loadTilesFromDir(const DirectoryPath&, Vec2 size, bool overwrite);
  void loadScriptedTextures(const DirectoryPath&, const FilePath&);
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
