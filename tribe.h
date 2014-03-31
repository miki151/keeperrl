#ifndef _TRIBE_H
#define _TRIBE_H

#include <map>
#include <set>

#include "enums.h"
#include "creature.h"
#include "event.h"
#include "singleton.h"

class Tribe : public EventListener {
  public:
  virtual double getStanding(const Creature*) const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;
  virtual void onAttackEvent(const Creature* victim, const Creature* attacker) override;

  void onItemsStolen(const Creature* thief);
  void makeSlightEnemy(const Creature*);
  void addMember(const Creature*);
  void removeMember(const Creature*);
  void setLeader(const Creature*);
  const Creature* getLeader();
  vector<const Creature*> getMembers(bool includeDead = false);
  const string& getName();
  void addEnemy(Tribe*);
  int getHandicap() const;
  void setHandicap(int);

  SERIALIZATION_DECL(Tribe);

  static void init();

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  Tribe(const string& name, bool diplomatic);

  private:

  bool SERIAL(diplomatic);

  void initStanding(const Creature*);
  double getMultiplier(const Creature* member);

  unordered_map<const Creature*, double> SERIAL(standing);
  vector<pair<const Creature*, const Creature*>> SERIAL(attacks);
  const Creature* SERIAL2(leader, nullptr);
  vector<const Creature*> SERIAL(members);
  unordered_set<Tribe*> SERIAL(enemyTribes);
  string SERIAL(name);
  int SERIAL2(handicap, 0);
};

typedef Singleton<Tribe, TribeId> Tribes;

#endif
