#include "stdafx.h"
#include "tile_paths.h"
#include "game_config.h"

TilePaths::TilePaths(vector<TileInfo> d, string mod) : definitions(std::move(d)), mainMod(std::move(mod)) {}

void TilePaths::merge(TilePaths other) {
  auto contains = [&] (const string& viewId) {
    for (auto& tile : definitions)
      if (tile.viewId == viewId)
        return true;
    return false;
  };
  for (auto& tile : other.definitions)
    if (!contains(tile.viewId))
      definitions.push_back(tile);
  for (auto& mod : concat({other.mainMod}, other.mergedMods))
    if (mod != mainMod && !mergedMods.contains(mod))
      mergedMods.push_back(mod);
}
