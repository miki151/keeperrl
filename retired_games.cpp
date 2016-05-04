#include "stdafx.h"
#include "retired_games.h"


const vector<RetiredGames::RetiredGame>& RetiredGames::getAllGames() const {
  return games;
}

void RetiredGames::setActive(int num, bool b) {
  if (b)
    active.insert(num);
  else
    active.erase(num);
}

bool RetiredGames::isActive(int num) const {
  return active.count(num);
}

int RetiredGames::getNumActive() const {
  return active.size();
}

vector<RetiredGames::RetiredGame> RetiredGames::getActiveGames() const {
  vector<RetiredGame> ret;
  for (int i : active)
    ret.push_back(games[i]);
  return ret;
}

void RetiredGames::addLocal(const SavedGameInfo& game, const SaveFileInfo& file) {
  games.push_back({game, file, 0, 0});
  ++numLocal;
}

void RetiredGames::addOnline(const SavedGameInfo& game, const SaveFileInfo& file, int total, int won) {
  for (int i : All(games))
    if (games[i].fileInfo.filename == file.filename) {
      games[i].numTotal = total;
      games[i].numWon = won;
      return;
    }
  games.push_back({game, file, total, won});
}

void RetiredGames::sort() {
  auto compare = [] (const RetiredGame& g1, const RetiredGame& g2) { return g1.numTotal > g2.numTotal; };
  std::sort(games.begin(), games.begin() + numLocal, compare);
  std::sort(games.begin() + numLocal, games.end(), compare);
}

static optional<int> containsFilename(const vector<SaveFileInfo>& files, const string& filename) {
  for (int i : All(files))
    if (files[i].filename == filename)
      return i;
  return none;
}

int RetiredGames::getNumLocal() const {
  return numLocal;
}
