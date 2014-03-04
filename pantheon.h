#ifndef _PANTHEON_H
#define _PANTHEON_H

#include "util.h"
#include "gender.h"

enum class DeityHabitat {
  EARTH,
  STONE,
  FIRE,
  AIR,
  TREES,
  WATER,
  STARS };

enum class Epithet {
  CHANGE,
  COURAGE,
  CRAFTS,
  DARKNESS,
  DEATH,
  DEFENSE,
  DESTRUCTION,
  FEAR,
  FORTUNE,
  HEALTH,
  HUNTING,
  LIGHT,
  LIGHTNING,
  LOVE,
  MIND,
  NATURE,
  SECRETS,
  WAR,
  WEALTH,
  WINTER,
  WISDOM,
};

class Deity {
  public:
  Deity(const string& name, Gender gender, const vector<Epithet>& epithets, DeityHabitat habitat);
  string getName() const;
  Gender getGender() const;
  string getEpithets() const;
  string getHabitatString() const;
  DeityHabitat getHabitat() const;
  void onPrayer(Creature* c);

  static vector<Deity*> getDeities();
  static Deity* getDeity(DeityHabitat);

  SERIALIZATION_DECL(Deity);

  private:
  Deity(Deity&) {}
  string name;
  Gender gender;
  vector<Epithet> epithets;
  vector<Epithet> usedEpithets;
  DeityHabitat habitat;
};

#endif
