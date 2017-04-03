#include "stdafx.h"
#include "saved_game_info.h"
#include "text_serialization.h"

SERIALIZE_DEF(SavedGameInfo, minions, dangerLevel, name, progressCount);

template void SavedGameInfo::serialize(TextOutputArchive&, unsigned);
template void SavedGameInfo::serialize(TextInputArchive&, unsigned);

SERIALIZATION_CONSTRUCTOR_IMPL(SavedGameInfo);

SavedGameInfo::SavedGameInfo(const vector<MinionInfo>& m, double d, const string& n, int s) 
    : minions(m), dangerLevel(d), name(n), progressCount(s) {
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

int SavedGameInfo::getProgressCount() const {
  return progressCount;
}
