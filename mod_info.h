#pragma once

#include "util.h"

struct ModVersionInfo {
  SteamId steamId;
  int version;
  string compatibilityTag;
};

struct ModInfo {
  string name;
  string author;
  string description;
  ModVersionInfo versionInfo;
  int numGames;
  double rating = -1; // don't display if less than 0
  // Steam mod info:
  bool isSubscribed = false;
  bool isActive = false;
  bool isLocal = false;
  vector<string> actions;
};
