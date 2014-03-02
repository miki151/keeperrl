#ifndef _TRIBE_H
#define _TRIBE_H

#include <map>
#include <set>

#include "enums.h"
#include "creature.h"
#include "event.h"

class Tribe : public EventListener {
  public:
  Tribe(const string& name, bool diplomatic);

  virtual double getStanding(const Creature*) const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;
  virtual void onAttackEvent(const Creature* victim, const Creature* attacker) override;

  void onItemsStolen(const Creature* thief);
  void makeSlightEnemy(const Creature*);
  void addMember(const Creature*);
  void removeMember(const Creature*);
  void addImportantMember(const Creature*);
  vector<const Creature*> getImportantMembers(bool includeDead = false);
  vector<const Creature*> getMembers(bool includeDead = false);
  const string& getName();
  void addEnemy(Tribe*);

  static void init();

  static Tribe* monster;
  static Tribe* pest;
  static Tribe* wildlife;
  static Tribe* human;
  static Tribe* elven;
  static Tribe* dwarven;
  static Tribe* goblin;
  static Tribe* player;
  static Tribe* dragon;
  static Tribe* castleCellar;
  static Tribe* bandit;
  static Tribe* killEveryone;
  static Tribe* peaceful;

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar& BOOST_SERIALIZATION_NVP(monster)
      & BOOST_SERIALIZATION_NVP(pest)
      & BOOST_SERIALIZATION_NVP(wildlife)
      & BOOST_SERIALIZATION_NVP(human)
      & BOOST_SERIALIZATION_NVP(elven)
      & BOOST_SERIALIZATION_NVP(dwarven)
      & BOOST_SERIALIZATION_NVP(goblin)
      & BOOST_SERIALIZATION_NVP(player)
      & BOOST_SERIALIZATION_NVP(dragon)
      & BOOST_SERIALIZATION_NVP(castleCellar)
      & BOOST_SERIALIZATION_NVP(bandit)
      & BOOST_SERIALIZATION_NVP(killEveryone)
      & BOOST_SERIALIZATION_NVP(peaceful);
  }

  SERIALIZATION_DECL(Tribe);

  template <class Archive>
  static void registerTypes(Archive& ar);

  private:

  bool diplomatic;

  void initStanding(const Creature*);
  double getMultiplier(const Creature* member);

  unordered_map<const Creature*, double> standing;
  vector<pair<const Creature*, const Creature*>> attacks;
  vector<const Creature*> importantMembers;
  vector<const Creature*> members;
  unordered_set<Tribe*> enemyTribes;
  string name;
};

#endif
