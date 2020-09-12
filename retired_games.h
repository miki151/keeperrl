#pragma once

#include "util.h"
#include "saved_game_info.h"
#include "save_file_info.h"

class RetiredGames {
  public:
  void addLocal(const SavedGameInfo&, const SaveFileInfo&, bool active);
  void addOnline(const SavedGameInfo&, const SaveFileInfo&, int numTotal, int numWon, bool subscribed);

  void sort();

  struct RetiredGame {
    SavedGameInfo gameInfo;
    SaveFileInfo fileInfo;
    int numTotal;
    int numWon;
    bool active;
    bool subscribed;
  };

  const vector<RetiredGame>& getAllGames() const;
  vector<RetiredGame> getActiveGames() const;

  int getNumActive() const;
  int getNumLocal() const;

  void setActive(int num, bool);
  bool isActive(int num) const;
  void erase(int num);

  private:
  vector<RetiredGame> games;
  int numLocal = 0;
};

