#include "stdafx.h"
#include "technology.h"
#include "util.h"

template <class Archive> 
void Technology::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(cost)
    & BOOST_SERIALIZATION_NVP(prerequisites)
    & BOOST_SERIALIZATION_NVP(research);
}

SERIALIZABLE(Technology);

void Technology::init() {
  int base = 100;
  Technology::set(TechId::ALCHEMY, new Technology("alchemy", base, {}));
  Technology::set(TechId::ALCHEMY_ADV, new Technology("advanced alchemy", base, {TechId::ALCHEMY}, false));
  Technology::set(TechId::GOBLIN, new Technology("goblins", base, {}));
  Technology::set(TechId::OGRE, new Technology("ogres", base, {TechId::GOBLIN}));
  Technology::set(TechId::HUMANOID_MUT, new Technology("humanoid mutation",base,{TechId::OGRE}, false));
  Technology::set(TechId::GOLEM, new Technology("stone golems", base, {TechId::ALCHEMY}));
  Technology::set(TechId::GOLEM_ADV, new Technology("iron golems", base, {TechId::GOLEM}));
  Technology::set(TechId::GOLEM_MAS, new Technology("lava golems", base, {TechId::GOLEM_ADV, TechId::ALCHEMY_ADV}));
  Technology::set(TechId::BEAST, new Technology("beast taming", base, {}));
  Technology::set(TechId::BEAST_MUT, new Technology("beast mutation", base, {TechId::BEAST}, false));
  Technology::set(TechId::CRAFTING, new Technology("crafting", base, {}));
  Technology::set(TechId::IRON_WORKING, new Technology("iron working", base, {TechId::CRAFTING}));
  Technology::set(TechId::TWO_H_WEAP, new Technology("two-handed weapons", base, {TechId::IRON_WORKING}));
  Technology::set(TechId::TRAPS, new Technology("traps", base, {TechId::CRAFTING}));
  Technology::set(TechId::ARCHERY, new Technology("archery", base, {TechId::CRAFTING}));
  Technology::set(TechId::SPELLS, new Technology("sorcery", base, {}));
  Technology::set(TechId::SPELLS_ADV, new Technology("advanced sorcery", base, {TechId::SPELLS}));
  Technology::set(TechId::SPELLS_MAS, new Technology("master sorcery", base, {TechId::SPELLS_ADV}, false));
  Technology::set(TechId::NECRO, new Technology("necromancy", base, {TechId::SPELLS}));
  Technology::set(TechId::VAMPIRE, new Technology("vampirism", base, {TechId::NECRO}));
  Technology::set(TechId::VAMPIRE_ADV, new Technology("advanced vampirism", base, {TechId::VAMPIRE,
        TechId::SPELLS_MAS}));
}

bool Technology::canResearch() const {
  return research;
}

int Technology::getCost() const {
  return cost;
}

vector<Technology*> Technology::getNextTechs(const vector<Technology*>& current) {
  vector<Technology*> ret;
  for (Technology* t : Technology::getAll())
    if (t->canLearnFrom(current) && !contains(current, t))
      ret.push_back(t);
  return ret;
}

Technology::Technology(const string& n, int c, const vector<TechId>& pre, bool canR)
    : name(n), cost(c), research(canR) {
  for (TechId id : pre)
    prerequisites.push_back(Technology::get(id));
}

bool Technology::canLearnFrom(const vector<Technology*>& techs) const {
  vector<Technology*> myPre = prerequisites;
  for (Technology* t : techs)
    removeElementMaybe(myPre, t);
  return myPre.empty();
}

const string& Technology::getName() const {
  return name;
}

vector<Technology*> Technology::getSorted() {
  vector<Technology*> ret;
  while (ret.size() < getAll().size()) {
    append(ret, getNextTechs(ret));
  }
  return ret;
}

const vector<Technology*> Technology::getPrerequisites() const {
  return prerequisites;
}
  
const vector<Technology*> Technology::getAllowed() const {
  vector<Technology*> ret;
  for (Technology* t : getAll())
    if (contains(t->prerequisites, this))
      ret.push_back(t);
  return ret;
}
