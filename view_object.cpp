#include "stdafx.h"

using namespace std;

ViewObject::ViewObject(ViewId id, ViewLayer l, const string& d, bool _shadow)
    : resource_id(id), viewLayer(l), description(d), shadow(_shadow) {
  if (islower(description[0]))
    description[0] = toupper(description[0]);
}

bool ViewObject::castsShadow() const {
  return shadow;
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

void ViewObject::setHostile(bool s) {
  hostile = s;
}

bool ViewObject::isHostile() const {
  return hostile && *hostile;
}

bool ViewObject::isFriendly() const {
  return hostile && !(*hostile);
}

void ViewObject::setBlind(bool s) {
  blind = s;
}

void ViewObject::setInvisible(bool s) {
  invisible = s;
}

bool ViewObject::isInvisible() const {
  return invisible;
}

void ViewObject::setPoisoned(bool s) {
  poisoned = s;
}

bool ViewObject::isPoisoned() const {
  return poisoned;
}

void ViewObject::setPlayer(bool s) {
  player = s;
}

bool ViewObject::isPlayer() const {
  return player;
}

void ViewObject::setHidden(bool s) {
  hidden = s;
}

bool ViewObject::isHidden() const {
  return hidden;
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
  if (attack && stats)
    attr = " att: " + convertToString(*attack) + " def: " + convertToString(*defense) + " ";
  vector<string> mods;
  if (getBleeding() > 0) 
    mods.push_back("wounded");
  if (blind)
    mods.push_back("blind");
  if (poisoned)
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
  return attack;
}

Optional<int> ViewObject::getDefense() const {
  return defense;
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

