#include "stdafx.h"
#include "retired_games.h"


RetiredGames::RetiredGames(const vector<SavedGameInfo>& g, const vector<SaveFileInfo>& f) : games(g), files(f) {
}

const vector<SavedGameInfo>& RetiredGames::getAllGames() const {
  return games;
}

const vector<SaveFileInfo>& RetiredGames::getAllFiles() const {
  return files;
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

vector<SavedGameInfo> RetiredGames::getActiveGames() const {
  vector<SavedGameInfo> ret;
  for (int i : active)
    ret.push_back(games[i]);
  return ret;
}

vector<SaveFileInfo> RetiredGames::getActiveFiles() const {
  vector<SaveFileInfo> ret;
  for (int i : active)
    ret.push_back(files[i]);
  return ret;
}

