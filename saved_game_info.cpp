#include "stdafx.h"
#include "saved_game_info.h"

SERIALIZE_DEF(SavedGameInfo, minions, dangerLevel, name);
#ifdef PARSE_GAME
template void SavedGameInfo::serialize(text_oarchive&, unsigned);
#endif

SERIALIZATION_CONSTRUCTOR_IMPL(SavedGameInfo);

SavedGameInfo::SavedGameInfo(const vector<MinionInfo>& m, double d, const string& n) : minions(m), dangerLevel(d),
  name(n) {
}

const vector<SavedGameInfo::MinionInfo>& SavedGameInfo::getMinions() const {
  return minions;
}

double SavedGameInfo::getDangerLevel() const {
  return dangerLevel;
}

const string& SavedGameInfo::getName() const {
  return name;
}

ViewId SavedGameInfo::getViewId() const {
  return minions.at(0).viewId;
}

