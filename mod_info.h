#pragma once

#include "util.h"

struct ModVersionInfo {
  SteamId steamId;
  int version;
  string compatibilityTag;
};

struct ModDetails {
  string author;
  string description;
};

struct ModInfo {
  string name;
  ModDetails details;
  ModVersionInfo versionInfo;
  int upvotes = 0;
  int downvotes = 0;
  bool canUpload = false;
  bool isSubscribed = false;
  bool isActive = false;
  bool isLocal = false;
  vector<string> actions;
};
