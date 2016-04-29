#ifndef _RETIRED_GAMES_H
#define _RETIRED_GAMES_H

#include "util.h"
#include "saved_game_info.h"
#include "save_file_info.h"

class RetiredGames {
  public:
  void addLocal(const SavedGameInfo&, const SaveFileInfo&);
  void addOnline(const SavedGameInfo&, const SaveFileInfo&, int numTotal, int numWon);

  void sort();

  struct RetiredGame {
    SavedGameInfo gameInfo;
    SaveFileInfo fileInfo;
    int numTotal;
    int numWon;
  };

  const vector<RetiredGame>& getAllGames() const;
  vector<RetiredGame> getActiveGames() const;

  int getNumActive() const;
  int getNumLocal() const;

  void setActive(int num, bool);
  bool isActive(int num) const;

  private:
  vector<RetiredGame> games;
  set<int> active;
  int numLocal = 0;
};

#endif
