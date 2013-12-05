#ifndef _TRIBE_H
#define _TRIBE_H

#include <map>
#include <set>

#include "enums.h"
#include "creature.h"
#include "event.h"

class Tribe : public EventListener {
  public:
  virtual double getStanding(const Creature*) const;

  Tribe(const string& name, bool diplomatic);

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;
  virtual void onAttackEvent(const Creature* victim, const Creature* attacker) override;

  void onItemsStolen(const Creature* thief);
  void makeSlightEnemy(const Creature*);
  void addImportantMember(const Creature*);
  vector<const Creature*> getImportantMembers();
  const string& getName();
  void addEnemy(Tribe*);

  static void init();

  static Tribe* const monster;
  static Tribe* const pest;
  static Tribe* const wildlife;
  static Tribe* const elven;
  static Tribe* const dwarven;
  static Tribe* const goblin;
  static Tribe* const player;
  static Tribe* const vulture;
  static Tribe* const killEveryone;

  private:

  bool diplomatic;

  void initStanding(const Creature*);
  double getMultiplier(const Creature* member);

  unordered_map<const Creature*, double> standing;
  vector<pair<const Creature*, const Creature*>> attacks;
  vector<const Creature*> importantMembers;
  unordered_set<Tribe*> enemyTribes;
  string name;
};

#endif
