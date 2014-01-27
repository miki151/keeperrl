#ifndef _QUEST_H
#define _QUEST_H

#include <map>
#include <string>

#include "util.h"

class Location;
class Tribe;

class Quest {
  public:
  virtual bool isFinished() const = 0;
  virtual string getMessage() const = 0;
  void addAdventurer(const Creature* c);
  void setLocation(const Location*);
  const Location* getLocation() const;

  static Quest* dragon;
  static Quest* castleCellar;
  static Quest* bandits;

  static Quest* killTribeQuest(Tribe* tribe, string message, bool onlyImp = false);


  protected:
  unordered_set<const Creature*> adventurers;
  const Location* location = nullptr;
};

#endif
