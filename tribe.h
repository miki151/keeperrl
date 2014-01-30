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
  void addMember(const Creature*);
  void addImportantMember(const Creature*);
  vector<const Creature*> getImportantMembers();
  vector<const Creature*> getMembers();
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
  static Tribe* vulture;
  static Tribe* dragon;
  static Tribe* castleCellar;
  static Tribe* bandit;
  static Tribe* killEveryone;

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
