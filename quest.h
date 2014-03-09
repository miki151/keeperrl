#ifndef _QUEST_H
#define _QUEST_H

#include <map>
#include <string>

#include "util.h"
#include "singleton.h"

class Location;
class Tribe;

class Quest {
  public:
  virtual ~Quest() {}
  virtual bool isFinished() const = 0;
  void addAdventurer(Creature* c);
  void setLocation(const Location*);
  
  static Quest* killTribeQuest(Tribe* tribe, string message, bool onlyImp = false);

  SERIALIZATION_DECL(Quest);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  Quest(const string& startMessage);
  unordered_set<const Creature*> adventurers;
  const Location* location = nullptr;
  string startMessage;
};

typedef Singleton<Quest, QuestId> Quests;

#endif
