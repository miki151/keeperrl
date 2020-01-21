#include "stdafx.h"
#include "tile_paths.h"
#include "game_config.h"

TilePaths::TilePaths(vector<TileInfo> d, vector<string> mods) : definitions(std::move(d)), mainMods(std::move(mods)) {}

void TilePaths::merge(TilePaths other) {
  auto contains = [&] (auto viewId) {
    for (auto& tile : definitions)
      if (tile.viewId == viewId)
        return true;
    return false;
  };
  for (auto& tile : other.definitions)
    if (!contains(tile.viewId))
      definitions.push_back(tile);
  for (auto& mod : concat(other.mainMods, other.mergedMods))
    if (!mainMods.contains(mod) && !mergedMods.contains(mod))
      mergedMods.push_back(mod);
}
