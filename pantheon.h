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

  static Deity* getDeity(DeityHabitat);
  static vector<Deity*> getDeities();

  SERIALIZATION_DECL(Deity);

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar & deities;
  }

  private:
  static vector<Deity*> deities;
  Deity(Deity&) {}
  string SERIAL(name);
  Gender SERIAL(gender);
  vector<Epithet> SERIAL(epithets);
  vector<Epithet> SERIAL(usedEpithets);
  DeityHabitat SERIAL(habitat);
};

#endif
