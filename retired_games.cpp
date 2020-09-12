#include "stdafx.h"
#include "retired_games.h"


const vector<RetiredGames::RetiredGame>& RetiredGames::getAllGames() const {
  return games;
}

void RetiredGames::setActive(int num, bool b) {
  games[num].active = b;
}

bool RetiredGames::isActive(int num) const {
  return games[num].active;
}

int RetiredGames::getNumActive() const {
  int cnt = 0;
  for (auto& game : games)
    if (game.active)
      ++cnt;
  return cnt;
}

vector<RetiredGames::RetiredGame> RetiredGames::getActiveGames() const {
  vector<RetiredGame> ret;
  for (auto& game : games)
    if (game.active)
      ret.push_back(game);
  return ret;
}

void RetiredGames::addLocal(const SavedGameInfo& game, const SaveFileInfo& file, bool active) {
  games.push_back({game, file, 0, 0, active, false});
  ++numLocal;
}

void RetiredGames::addOnline(const SavedGameInfo& game, const SaveFileInfo& file, int total, int won, bool subscribed) {
  for (int i : All(games))
    if (games[i].fileInfo.filename == file.filename) {
      games[i].numTotal = total;
      games[i].numWon = won;
      return;
    }
  games.push_back({game, file, total, won, false, subscribed});
}

void RetiredGames::sort() {
  auto compare = [] (const RetiredGame& g1, const RetiredGame& g2) {
    return std::forward_as_tuple(g1.subscribed, g1.numTotal) > std::forward_as_tuple(g2.subscribed, g2.numTotal); };
  std::sort(games.begin(), games.begin() + numLocal, compare);
  std::sort(games.begin() + numLocal, games.end(), compare);
}

int RetiredGames::getNumLocal() const {
  return numLocal;
}

void RetiredGames::erase(int num) {
  games.removeIndexPreserveOrder(num);
  if (num < numLocal)
    --numLocal;
}