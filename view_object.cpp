#include "stdafx.h"

#include "view_object.h"

template <class Archive> 
void ViewObject::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(bleeding)
    & BOOST_SERIALIZATION_NVP(enemyStatus)
    & BOOST_SERIALIZATION_NVP(resource_id)
    & BOOST_SERIALIZATION_NVP(viewLayer)
    & BOOST_SERIALIZATION_NVP(description)
    & BOOST_SERIALIZATION_NVP(burning)
    & BOOST_SERIALIZATION_NVP(height)
    & BOOST_SERIALIZATION_NVP(sizeIncrease)
    & BOOST_SERIALIZATION_NVP(modifiers)
    & BOOST_SERIALIZATION_NVP(attack)
    & BOOST_SERIALIZATION_NVP(defense)
    & BOOST_SERIALIZATION_NVP(waterDepth);
}

SERIALIZABLE(ViewObject);

ViewObject::ViewObject(ViewId id, ViewLayer l, const string& d)
    : resource_id(id), viewLayer(l), description(d) {
  if (islower(description[0]))
    description[0] = toupper(description[0]);
}

ViewObject& ViewObject::setModifier(Modifier mod) {
  modifiers.insert(mod);
  return *this;
}

ViewObject& ViewObject::removeModifier(Modifier mod) {
  modifiers.erase(mod);
  return *this;
}

bool ViewObject::hasModifier(Modifier mod) const {
  return modifiers.count(mod);
}

ViewObject& ViewObject::setWaterDepth(double depth) {
  waterDepth = depth;
  return *this;
}

double ViewObject::getWaterDepth() const {
  return waterDepth;
}

void ViewObject::setBleeding(double b) {
  bleeding = b;
}

void ViewObject::setEnemyStatus(EnemyStatus s) {
  enemyStatus = s;
}

bool ViewObject::isHostile() const {
  return enemyStatus == HOSTILE;
}

bool ViewObject::isFriendly() const {
  return enemyStatus == FRIENDLY;
}

void ViewObject::setBurning(double s) {
  burning = s;
}

double ViewObject::getBurning() const {
  return burning;
}

double ViewObject::getBleeding() const {
  return bleeding;
}

void ViewObject::setHeight(double h) {
  height = h;
}

double ViewObject::getHeight() const {
  return height;
}

void ViewObject::setSizeIncrease(double s) {
  sizeIncrease += s;
}

double ViewObject::getSizeIncrease() const {
  return sizeIncrease;
}

string ViewObject::getBareDescription() const {
  return description;
}

string ViewObject::getDescription(bool stats) const {
  string attr;
  if (attack > -1 && stats)
    attr = " att: " + convertToString(attack) + " def: " + convertToString(defense) + " ";
  vector<string> mods;
  if (getBleeding() > 0) 
    mods.push_back("wounded");
  if (hasModifier(BLIND))
    mods.push_back("blind");
  if (hasModifier(POISONED))
    mods.push_back("poisoned");
  if (mods.size() > 0)
    return description + attr + "(" + combine(mods) + ")";
  else
    return description + attr;
}

void ViewObject::setAttack(int val) {
  attack = val;
}

void ViewObject::setDefense(int val) {
  defense = val;
}

Optional<int> ViewObject::getAttack() const {
  if (attack > -1)
    return attack;
  else
    return Nothing();
}

Optional<int> ViewObject::getDefense() const {
  if (defense > -1)
    return defense;
  else
    return Nothing();
}

ViewLayer ViewObject::layer() const {
  return viewLayer;
}


static vector<ViewId> creatureIds {
  ViewId::PLAYER,
  ViewId::ELF,
  ViewId::ELF_CHILD,
  ViewId::ELF_LORD,
  ViewId::ELVEN_SHOPKEEPER,
  ViewId::DWARF,
  ViewId::DWARF_BARON,
  ViewId::DWARVEN_SHOPKEEPER,
  ViewId::GREAT_GOBLIN,
  ViewId::GOBLIN,
  ViewId::BANDIT,
  ViewId::ZOMBIE,
  ViewId::VAMPIRE,
  ViewId::MUMMY,
  ViewId::MUMMY_LORD,
  ViewId::HORSE,
  ViewId::COW,
  ViewId::PIG,
  ViewId::SHEEP,
  ViewId::JACKAL,
  ViewId::DEER,
  ViewId::BOAR,
  ViewId::FOX,
  ViewId::BEAR,
  ViewId::WOLF,
  ViewId::BAT,
  ViewId::RAT,
  ViewId::SNAKE,
  ViewId::VULTURE,
  ViewId::GNOME,
  ViewId::VODNIK,
  ViewId::LEPRECHAUN,
  ViewId::KRAKEN,
  ViewId::KRAKEN2,
  ViewId::FIRE_SPHERE,
  ViewId::NIGHTMARE,
  ViewId::DEATH,
  ViewId::SPECIAL_BEAST,
  ViewId::SPECIAL_HUMANOID,
  ViewId::CANIF_TREE,
  ViewId::DECID_TREE,
};

static vector<ViewId> itemIds {
  ViewId::BODY_PART,
  ViewId::BONE,
  ViewId::SWORD,
  ViewId::ELVEN_SWORD,
  ViewId::KNIFE,
  ViewId::WAR_HAMMER,
  ViewId::BATTLE_AXE,
  ViewId::BOW,
  ViewId::ARROW,
  ViewId::SCROLL,
  ViewId::STEEL_AMULET,
  ViewId::COPPER_AMULET,
  ViewId::CRYSTAL_AMULET,
  ViewId::WOODEN_AMULET,
  ViewId::AMBER_AMULET,
  ViewId::BOOK,
  ViewId::FIRST_AID,
  ViewId::EFFERVESCENT_POTION,
  ViewId::MURKY_POTION,
  ViewId::SWIRLY_POTION,
  ViewId::VIOLET_POTION,
  ViewId::PUCE_POTION,
  ViewId::SMOKY_POTION,
  ViewId::FIZZY_POTION,
  ViewId::MILKY_POTION,
  ViewId::GOLD,
  ViewId::LEATHER_ARMOR,
  ViewId::LEATHER_HELM,
  ViewId::CHAIN_ARMOR,
  ViewId::IRON_HELM,
  ViewId::BOULDER,
  ViewId::ROCK,
  ViewId::SLIMY_MUSHROOM, 
  ViewId::PINK_MUSHROOM, 
  ViewId::DOTTED_MUSHROOM, 
  ViewId::GLOWING_MUSHROOM, 
  ViewId::GREEN_MUSHROOM, 
  ViewId::BLACK_MUSHROOM
};

static bool hallu = false;

void ViewObject::setHallu(bool b) {
  hallu = b;
}

ViewId ViewObject::id() const {
  if (hallu) {
    if (contains(creatureIds, resource_id))
      return creatureIds[Random.getRandom(creatureIds.size())];
    if (contains(itemIds, resource_id))
      return itemIds[Random.getRandom(itemIds.size())];
  }
  return resource_id;
}

const ViewObject& ViewObject::unknownMonster() {
  static ViewObject ret(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE, "Unknown creature");
  return ret;
}

const ViewObject& ViewObject::empty() {
  static ViewObject ret(ViewId::BORDER_GUARD, ViewLayer::FLOOR, "");
  return ret;
}

const ViewObject& ViewObject::mana() {
  static ViewObject ret(ViewId::MANA, ViewLayer::FLOOR, "");
  return ret;
}

