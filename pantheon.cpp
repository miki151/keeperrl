#include "stdafx.h"

#include "pantheon.h"
#include "creature.h"
#include "item_factory.h"
#include "creature_factory.h"
#include "level.h"


template <class Archive> 
void Deity::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(gender)
    & BOOST_SERIALIZATION_NVP(epithets)
    & BOOST_SERIALIZATION_NVP(usedEpithets)
    & BOOST_SERIALIZATION_NVP(habitat);
}

SERIALIZABLE(Deity);

static map<DeityHabitat, vector<Epithet>> epithetsMap {
  { DeityHabitat::FIRE,
      { Epithet::WAR, Epithet::DEATH, Epithet::DESTRUCTION, Epithet::WEALTH,
        Epithet::FEAR, Epithet::CRAFTS, Epithet::LIGHT, Epithet::DARKNESS }},
  { DeityHabitat::EARTH,
      { Epithet::HEALTH, Epithet::NATURE, Epithet::WINTER, Epithet::LOVE,
        Epithet::WEALTH, Epithet::MIND, Epithet::CHANGE, Epithet::DEFENSE, Epithet::DARKNESS }},
  { DeityHabitat::TREES,
      { Epithet::HEALTH, Epithet::NATURE, Epithet::LOVE, Epithet::LIGHT,
        Epithet::CHANGE, Epithet::CRAFTS, Epithet::HUNTING, Epithet::FORTUNE, Epithet::SECRETS,  }},
  { DeityHabitat::STONE,
      { Epithet::WISDOM, Epithet::WEALTH, Epithet::DEFENSE, Epithet::SECRETS, Epithet::DEATH,
        Epithet::WINTER, }},
  { DeityHabitat::WATER,
      { Epithet::NATURE, Epithet::WISDOM, Epithet::WEALTH, Epithet::MIND, Epithet::DESTRUCTION,
        Epithet::CHANGE, Epithet::DEFENSE, Epithet::FEAR, Epithet::COURAGE, Epithet::HEALTH,  }},
  { DeityHabitat::AIR,
      { Epithet::MIND, Epithet::LIGHTNING, Epithet::LIGHT, Epithet::CHANGE,
        Epithet::FORTUNE, Epithet::FEAR, Epithet::COURAGE,  }},
  { DeityHabitat::STARS,
      { Epithet::DEATH, Epithet::WAR, Epithet::WINTER, Epithet::WISDOM, Epithet::LOVE,
        Epithet::DARKNESS, Epithet::FORTUNE, Epithet::SECRETS,  }}

};

/*
    CHANGE earth, water, trees, earth
    COURAGE air, water
    CRAFTS trees, fire
    DARKNESS stars, earth, fire
    DEATH stars, fire, stone
    DEFENSE water, stone, earth
    DESTRUCTION water, fire
    FEAR air, water, fire
    FORTUNE stars, air, trees
    HEALTH water, trees, earth
    HUNTING trees
    LIGHT air, trees, fire
    LIGHTNING air
    LOVE stars, air, trees
    MIND air, water, earth
    NATURE water, trees, earth
    SECRETS stars, stone, trees
    WAR stars, fire
    WEALTH water, stone, earth, fire
    WINTER stars, stone, earth
    WISDOM stars, water, stone
*/

static string getEpithetString(Epithet epithet) {
  switch (epithet) {
    case Epithet::CHANGE: return "change";
    case Epithet::COURAGE: return "courage"; // ?
    case Epithet::CRAFTS: return "crafts";
    case Epithet::DARKNESS: return "darkness";
    case Epithet::DEATH: return "death";
    case Epithet::DEFENSE: return "defense";
    case Epithet::DESTRUCTION: return "destruction";
    case Epithet::FEAR: return "fear";
    case Epithet::FORTUNE: return "fortune"; // ?
    case Epithet::HEALTH: return "health";
    case Epithet::HUNTING: return "hunting"; // cos do polowania
    case Epithet::LIGHT: return "light"; // ?
    case Epithet::LIGHTNING: return "lightning";
    case Epithet::LOVE: return "love"; // potion of love
    case Epithet::MIND: return "mind";
    case Epithet::NATURE: return "nature";
    case Epithet::SECRETS: return "secrets";
    case Epithet::WAR: return "war";
    case Epithet::WEALTH: return "wealth";
    case Epithet::WINTER: return "winter"; // zamarza level
    case Epithet::WISDOM: return "wisdom";
  }
  return "";
}

string Deity::getName() const {
  return name;
}

Gender Deity::getGender() const {
  return gender;
}

string Deity::getEpithets() const {
  vector<string> v;
  for (Epithet e : epithets)
    v.push_back(getEpithetString(e));
  return combine(v);
}

static string toString(DeityHabitat h) {
  switch (h) {
    case DeityHabitat::EARTH: return "the earth";
    case DeityHabitat::STONE: return "stone";
    case DeityHabitat::FIRE: return "fire";
    case DeityHabitat::AIR: return "the air";
    case DeityHabitat::TREES: return "the trees";
    case DeityHabitat::WATER: return "water";
    case DeityHabitat::STARS: return "the stars";
  }
  return "";
}

DeityHabitat Deity::getHabitat() const {
  return habitat;
}

string Deity::getHabitatString() const {
  return toString(habitat);
}

static void grantGift(Creature* c, ItemId id, string deity, int num = 1) {
  c->privateMessage(deity + " grants you a gift.");
  c->takeItems(ItemFactory::fromId(id, num), nullptr);
}

static void applyEffect(Creature* c, EffectType effect, string msg) {
  c->privateMessage(msg);
  Effect::applyToCreature(c, effect, EffectStrength::STRONG);
}

void Deity::onPrayer(Creature* c) {
  bool prayerAnswered = false;
  for (Epithet epithet : randomPermutation(epithets)) {
    if (contains(usedEpithets, epithet))
      continue;
    bool noEffect = false;
    switch (epithet) {
      case Epithet::DEATH: {
          PCreature death = CreatureFactory::fromId(CreatureId::DEATH, Tribe::killEveryone);
          for (Vec2 v : c->getPosition().neighbors8(true))
            if (c->getLevel()->inBounds(v) && c->getLevel()->getSquare(v)->canEnter(death.get())) {
              c->privateMessage("Death appears before you.");
              c->getLevel()->addCreature(v, std::move(death));
              break;
            }
          if (death)
            noEffect = true;
          break; }
      case Epithet::WAR:
          grantGift(c, chooseRandom(
          {ItemId::SPECIAL_SWORD, ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}), name); break;
      case Epithet::WISDOM: grantGift(c, 
          chooseRandom({ItemId::MUSHROOM_BOOK, ItemId::POTION_BOOK, ItemId::AMULET_BOOK}), name); break;
      case Epithet::DESTRUCTION: applyEffect(c, EffectType::DESTROY_EQUIPMENT, ""); break;
      case Epithet::SECRETS: grantGift(c, ItemId::INVISIBLE_POTION, name); break;
      case Epithet::LIGHTNING:
          c->bleed(0.9);
          c->you(MsgType::ARE, "struck by a lightning bolt!");
          break;
      case Epithet::FEAR: applyEffect(c, EffectType::PANIC, name + " puts fear in your heart"); break;
      case Epithet::MIND: 
          if (Random.roll(2))
            applyEffect(c, EffectType::RAGE, name + " fills your head with anger");
          else
            applyEffect(c, EffectType::HALLU, "");
          break;
      case Epithet::CHANGE:
          if (Random.roll(2) && c->getEquipment().getItem(EquipmentSlot::WEAPON)) {
            PCreature snake = CreatureFactory::fromId(CreatureId::SNAKE, Tribe::pest);
            for (Vec2 v : c->getPosition().neighbors8(true))
              if (c->getLevel()->inBounds(v) && c->getLevel()->getSquare(v)->canEnter(snake.get())) {
                c->getLevel()->addCreature(v, std::move(snake));
                c->steal({c->getEquipment().getItem(EquipmentSlot::WEAPON)});
                c->privateMessage("Ouch!");
                c->you(MsgType::YOUR, "weapon turns into a snake!");
                break;
              }
            if (!snake)
              break;
          }
          for (Item* it : randomPermutation(c->getEquipment().getItems())) {
            if (it->getType() == ItemType::POTION) {
              c->privateMessage("Your " + it->getName() + " changes color!");
              c->steal({it});
              c->take(ItemFactory::potions().random());
              break;
            }
            if (it->getType() == ItemType::SCROLL) {
              c->privateMessage("Your " + it->getName() + " changes label!");
              c->steal({it});
              c->take(ItemFactory::scrolls().random());
              break;
            }
            if (it->getType() == ItemType::AMULET) {
              c->privateMessage("Your " + it->getName() + " changes shape!");
              c->steal({it});
              c->take(ItemFactory::amulets().random());
              break;
            }
          }
          break;
      case Epithet::HEALTH:
          if (c->getHealth() < 1 || c->lostLimbs())
            applyEffect(c, EffectType::HEAL, "You feel a healing power overcoming you");
          else {
            if (Random.roll(4))
              grantGift(c, ItemId::HEALING_AMULET, name);
            else
              grantGift(c, ItemId::HEALING_POTION, name, Random.getRandom(1, 4));
          }
          break;
      case Epithet::NATURE: grantGift(c, ItemId::FRIENDLY_ANIMALS_AMULET, name); break;
//      case Epithet::LOVE: grantGift(c, ItemId::PANIC_MUSHROOM, name); break;
      case Epithet::WEALTH: grantGift(c, ItemId::GOLD_PIECE, name, Random.getRandom(100, 200)); break;
      case Epithet::DEFENSE: grantGift(c, ItemId::DEFENSE_AMULET, name); break;
      case Epithet::DARKNESS: applyEffect(c, EffectType::BLINDNESS, ""); break;
      case Epithet::CRAFTS: applyEffect(c,
          chooseRandom({EffectType::ENHANCE_ARMOR, EffectType::ENHANCE_WEAPON}), ""); break;
//      case Epithet::HUNTING: grantGift(c, ItemId::PANIC_MUSHROOM, name); break;
      default: noEffect = true;
    }
    usedEpithets.push_back(epithet);
    if (!noEffect) {
      prayerAnswered = true;
      break;
    }
  }
  if (!prayerAnswered)
    c->privateMessage("Your prayer is not answered.");
}

vector<Deity*> generateDeities() {
  set<Epithet> used;
  vector<Deity*> ret;
  for (auto elem : epithetsMap) {
    string deity = NameGenerator::deityNames.getNext();
    Gender gend = Gender::MALE;
    if ((deity.back() == 'a' || deity.back() == 'i') && !Random.roll(4))
      gend = Gender::FEMALE;
    vector<Epithet> ep;
    for (int i : Range(Random.getRandom(1, 4))) {
      Epithet epithet;
      int cnt = 100;
      do {
        epithet = chooseRandom(elem.second);
      } while (used.count(epithet) && --cnt > 0);
      if (cnt == 0)
        break;
      used.insert(epithet);
      ep.push_back(epithet);
    }
    if (ep.empty())
      return generateDeities();
    ret.push_back(new Deity(deity, gend, ep, elem.first)); 
  }
  return ret;
}

vector<Deity*> Deity::getDeities() {
  static vector<Deity*> deities = generateDeities();
  return deities;
}

Deity* Deity::getDeity(DeityHabitat h) {
  for (Deity* d : getDeities())
    if (d->getHabitat() == h)
      return d;
  FAIL << "Didn't find deity for habitat " << (int)h;
  return getDeities()[0];
}
 
Deity::Deity(const string& n, Gender g, const vector<Epithet>& e, DeityHabitat h) :
    name(n), gender(g), epithets(e), habitat(h) {
}

