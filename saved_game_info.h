#ifndef _SAVED_GAME_INFO_H
#define _SAVED_GAME_INFO_H

#include "util.h"
#include "view_id.h"

class SavedGameInfo {
  public:

  struct MinionInfo {
    ViewId SERIAL(viewId);
    int SERIAL(level);
    SERIALIZE_ALL(viewId, level);
  };

  SavedGameInfo(const vector<MinionInfo>& minions, double dangerLevel, const string& name);
  const vector<MinionInfo>& getMinions() const;
  double getDangerLevel() const;
  const string& getName() const;
  ViewId getViewId() const;

  SERIALIZATION_DECL(SavedGameInfo);

  private:
  vector<MinionInfo> SERIAL(minions);
  double SERIAL(dangerLevel);
  string SERIAL(name);
};


#endif
