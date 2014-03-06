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
  void addAdventurer(Creature* c);
  void setLocation(const Location*);

  static Quest* dragon;
  static Quest* castleCellar;
  static Quest* bandits;
  static Quest* goblins;
  static Quest* dwarves;

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar& BOOST_SERIALIZATION_NVP(dragon)
      & BOOST_SERIALIZATION_NVP(castleCellar)
      & BOOST_SERIALIZATION_NVP(bandits)
      & BOOST_SERIALIZATION_NVP(goblins)
      & BOOST_SERIALIZATION_NVP(dwarves);
  }

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

#endif
