#include "stdafx.h"
#include "game_config.h"
#include "pretty_printing.h"

const char* GameConfig::getConfigName(GameConfigId id) {
  switch (id) {
    case GameConfigId::CAMPAIGN_VILLAINS:
      return "campaign_villains";
  }
}

string GameConfig::removeFormatting(string contents) {
  string ret;
  for (int i = 0; i < contents.size(); ++i) {
    if (contents[i] == '#') {
      while (contents[i] != '\n' && i < contents.size())
        ++i;
    }
    else if (contents[i] == '{')
      ret += " { ";
    else if (contents[i] == '}')
      ret += " } ";
    else
      ret += contents[i];
  }
  return ret;
}

GameConfig::GameConfig(DirectoryPath path) : path(std::move(path)) {
}
