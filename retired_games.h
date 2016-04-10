#ifndef _RETIRED_GAMES_H
#define _RETIRED_GAMES_H

#include "util.h"
#include "saved_game_info.h"
#include "save_file_info.h"

class RetiredGames {
  public:
  RetiredGames(const vector<SavedGameInfo>&, const vector<SaveFileInfo>&);
  const vector<SavedGameInfo>& getAllGames() const;
  const vector<SaveFileInfo>& getAllFiles() const;

  int getNumActive() const;
  vector<SavedGameInfo> getActiveGames() const;
  vector<SaveFileInfo> getActiveFiles() const;

  void setActive(int num, bool);
  bool isActive(int num) const;

  private:
  vector<SavedGameInfo> games;
  vector<SaveFileInfo> files;
  set<int> active;
};

#endif
