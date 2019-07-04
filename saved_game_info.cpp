#include "stdafx.h"
#include "saved_game_info.h"
#include "text_serialization.h"
#include "parse_game.h"

ViewId SavedGameInfo::getViewId() const {
  return minions[0].viewId;
}
