#pragma once

#include "util.h"

struct ModInfo {
  string name;
  string author;
  string description;
  int numGames;
  int version;
  double rating = -1; // don't display if less than 0
  // Steam mod info:
  SteamId steamId;
  bool isSubscribed = false;
  bool isActive = false;
  bool isLocal = false;
  vector<string> actions;
};
