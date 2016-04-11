#ifndef _SAVED_GAME_INFO_H
#define _SAVED_GAME_INFO_H

#include "util.h"
#include "view_id.h"

class SavedGameInfo {
  public:
  SavedGameInfo(const Game*);

  struct MinionInfo {
    ViewId SERIAL(viewId);
    int SERIAL(level);
    SERIALIZE_ALL(viewId, level);
  };

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
