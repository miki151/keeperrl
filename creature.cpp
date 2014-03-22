#include "stdafx.h"

#include "creature.h"
#include "creature_factory.h"
#include "level.h"
#include "enemy_check.h"
#include "ranged_weapon.h"
#include "statistics.h"
#include "options.h"

template <class Archive> 
void Creature::Vision::serialize(Archive& ar, const unsigned int version) {
}

SERIALIZABLE(Creature::Vision);

template <class Archive> 
void SpellInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(id)
    & BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(ready)
    & BOOST_SERIALIZATION_NVP(difficulty);
}

SERIALIZABLE(SpellInfo);


template <class Archive> 
void Creature::serialize(Archive& ar, const unsigned int version) { 
  ar
    & SUBCLASS(CreatureAttributes)
    & SUBCLASS(CreatureView)
    & SUBCLASS(UniqueEntity)
    & SUBCLASS(EventListener)
    & BOOST_SERIALIZATION_NVP(viewObject)
    & BOOST_SERIALIZATION_NVP(level)
    & BOOST_SERIALIZATION_NVP(position)
    & BOOST_SERIALIZATION_NVP(time)
    & BOOST_SERIALIZATION_NVP(equipment)
    & BOOST_SERIALIZATION_NVP(shortestPath)
    & BOOST_SERIALIZATION_NVP(knownHiding)
    & BOOST_SERIALIZATION_NVP(tribe)
    & BOOST_SERIALIZATION_NVP(enemyChecks)
    & BOOST_SERIALIZATION_NVP(health)
    & BOOST_SERIALIZATION_NVP(dead)
    & BOOST_SERIALIZATION_NVP(lastTick)
    & BOOST_SERIALIZATION_NVP(collapsed)
    & BOOST_SERIALIZATION_NVP(injuredArms)
    & BOOST_SERIALIZATION_NVP(injuredLegs)
    & BOOST_SERIALIZATION_NVP(injuredWings)
    & BOOST_SERIALIZATION_NVP(injuredHeads)
    & BOOST_SERIALIZATION_NVP(lostArms)
    & BOOST_SERIALIZATION_NVP(lostLegs)
    & BOOST_SERIALIZATION_NVP(lostWings)
    & BOOST_SERIALIZATION_NVP(lostHeads)
    & BOOST_SERIALIZATION_NVP(hidden)
    & BOOST_SERIALIZATION_NVP(lastAttacker)
    & BOOST_SERIALIZATION_NVP(swapPositionCooldown)
    & BOOST_SERIALIZATION_NVP(lastingEffects)
    & BOOST_SERIALIZATION_NVP(stunned)
    & BOOST_SERIALIZATION_NVP(expLevel)
    & BOOST_SERIALIZATION_NVP(unknownAttacker)
    & BOOST_SERIALIZATION_NVP(visibleEnemies)
    & BOOST_SERIALIZATION_NVP(privateEnemies)
    & BOOST_SERIALIZATION_NVP(holding)
    & BOOST_SERIALIZATION_NVP(controller)
    & BOOST_SERIALIZATION_NVP(controllerStack)
    & BOOST_SERIALIZATION_NVP(visions)
    & BOOST_SERIALIZATION_NVP(kills)
    & BOOST_SERIALIZATION_NVP(difficultyPoints)
    & BOOST_SERIALIZATION_NVP(points);
}

SERIALIZABLE(Creature);

PCreature Creature::defaultCreature;

void Creature::initialize() {
  defaultCreature.reset();
}

Creature* Creature::getDefault() {
  if (!defaultCreature)
    defaultCreature = CreatureFactory::fromId(CreatureId::GNOME, Tribes::get(TribeId::MONSTER),
        MonsterAIFactory::idle());
  return defaultCreature.get();
}

bool increaseExperience = true;

void Creature::noExperienceLevels() {
  increaseExperience = false;
}

Creature::Creature(ViewObject object, Tribe* t, const CreatureAttributes& attr, ControllerFactory f)
    : CreatureAttributes(attr), viewObject(object), tribe(t), controller(f.get(this)) {
  tribe->addMember(this);
  for (Skill* skill : skills)
    skill->onTeach(this);
}

Creature::Creature(Tribe* t, const CreatureAttributes& attr, ControllerFactory f)
    : Creature(ViewObject(*attr.viewId, ViewLayer::CREATURE, *attr.name), t, attr, f) {}

Creature::~Creature() {
  tribe->removeMember(this);
}

ViewIndex Creature::getViewIndex(Vec2 pos) const {
  return level->getSquare(pos)->getViewIndex(this);
}

SpellInfo Creature::getSpell(SpellId id) {
  switch (id) {
    case SpellId::HEALING: return {id, "healing", EffectType::HEAL, 0, 30};
    case SpellId::SUMMON_INSECTS: return {id, "summon insects", EffectType::SUMMON_INSECTS, 0, 30};
    case SpellId::DECEPTION: return {id, "deception", EffectType::DECEPTION, 0, 60};
    case SpellId::SPEED_SELF: return {id, "haste self", EffectType::SPEED, 0, 60};
    case SpellId::STR_BONUS: return {id, "strength", EffectType::STR_BONUS, 0, 90};
    case SpellId::DEX_BONUS: return {id, "dexterity", EffectType::DEX_BONUS, 0, 90};
    case SpellId::FIRE_SPHERE_PET: return {id, "fire sphere", EffectType::FIRE_SPHERE_PET, 0, 20};
    case SpellId::TELEPORT: return {SpellId::TELEPORT, "escape", EffectType::TELEPORT, 0, 120};
    case SpellId::INVISIBILITY: return {SpellId::INVISIBILITY, "invisibility", EffectType::INVISIBLE, 0, 300};
    case SpellId::WORD_OF_POWER: return {SpellId::WORD_OF_POWER, "word of power", EffectType::WORD_OF_POWER, 0, 300};
  }
  FAIL << "wpeofk";
  return getSpell(SpellId::HEALING);
}

bool Creature::isFireResistant() const {
  return fireCreature || fireResistant;
}

void Creature::addSpell(SpellId id) {
  for (SpellInfo& info : spells)
    if (info.id == id)
      return;
  spells.push_back(getSpell(id));
}

const vector<SpellInfo>& Creature::getSpells() const {
  return spells;
}

bool Creature::canCastSpell(int index) const {
  CHECK(index >= 0 && index < spells.size());
  return spells[index].ready <= getTime();
}

void Creature::castSpell(int index) {
  CHECK(canCastSpell(index));
  Effect::applyToCreature(this, spells[index].type, EffectStrength::NORMAL);
  Statistics::add(StatId::SPELL_CAST);
  spells[index].ready = getTime() + spells[index].difficulty;
  spendTime(1);
}

void Creature::addVision(Vision* vision) {
  visions.push_back(vision);
}

void Creature::removeVision(Vision* vision) {
  removeElement(visions, vision);
}

void Creature::pushController(Controller* ctrl) {
  viewObject.setModifier(ViewObject::PLAYER);
  controllerStack.push_back(std::move(controller));
  controller.reset(ctrl);
}

void Creature::popController() {
  viewObject.removeModifier(ViewObject::PLAYER);
  CHECK(canPopController());
  bool wasPlayer = controller->isPlayer();
  controller = std::move(controllerStack.back());
  controllerStack.pop_back();
  if (wasPlayer && !controller->isPlayer())
    level->setPlayer(nullptr);
}

bool Creature::canPopController() const {
  return !controllerStack.empty();
}

bool Creature::isDead() const {
  return dead;
}

const Creature* Creature::getLastAttacker() const {
  return lastAttacker;
}

vector<const Creature*> Creature::getKills() const {
  return kills;
}

void Creature::spendTime(double t) {
  time += 100.0 * t / (double) getAttr(AttrType::SPEED);
  hidden = false;
}

bool Creature::canMove(Vec2 direction) const {
  if (holding || isAffected(ENTANGLED)) {
    privateMessage("You can't break free!");
    return false;
  }
  return (direction.length8() == 1 && level->canMoveCreature(this, direction)) || canSwapPosition(direction);
}

void Creature::move(Vec2 direction) {
  stationary = false;
  Debug() << getTheName() << " moving " << direction;
  CHECK(canMove(direction));
  if (level->canMoveCreature(this, direction))
    level->moveCreature(this, direction);
  else
    swapPosition(direction);
  if (collapsed) {
    you(MsgType::CRAWL, getConstSquare()->getName());
    spendTime(3);
  } else
    spendTime(1);
}

int Creature::getDebt(const Creature* debtor) const {
  return controller->getDebt(debtor);
}

void Creature::takeItems(vector<PItem> items, const Creature* from) {
  vector<Item*> ref = Item::extractRefs(items);
  getSquare()->dropItems(std::move(items));
  controller->onItemsAppeared(ref, from);
}

void Creature::you(MsgType type, const string& param) const {
  controller->you(type, param);
}

void Creature::you(const string& param) const {
  controller->you(param);
}

void Creature::privateMessage(const string& message) const {
  controller->privateMessage(message);
}

void Creature::grantIdentify(int numItems) {
  controller->grantIdentify(numItems);
}

Controller* Creature::getController() {
  return controller.get();
}

const MapMemory& Creature::getMemory(const Level* l) const {
  return controller->getMemory(l);
}

bool Creature::canSwapPosition(Vec2 direction) const {
  const Creature* c = getConstSquare(direction)->getCreature();
  if (!c)
    return false;
  if (c->isAffected(SLEEP)) {
    privateMessage(c->getTheName() + " is sleeping.");
    return false;
  }
  return (!swapPositionCooldown || isPlayer()) && !c->stationary && !c->invincible &&
      direction.length8() == 1 && !c->isPlayer() && !c->isEnemy(this) &&
      getConstSquare(direction)->canEnterEmpty(this) && getConstSquare()->canEnterEmpty(c);
}

void Creature::swapPosition(Vec2 direction) {
  CHECK(canSwapPosition(direction));
  swapPositionCooldown = 4;
  getConstSquare(direction)->getCreature()->privateMessage("Excuse me!");
  privateMessage("Excuse me!");
  level->swapCreatures(this, getSquare(direction)->getCreature());
}

void Creature::makeMove() {
  CHECK(!isDead());
  if (holding && holding->isDead())
    holding = nullptr;
  if (isAffected(SLEEP)) {
    controller->sleeping();
    spendTime(1);
    return;
  }
  if (stunned) {
    spendTime(1);
    stunned = false;
    return;
  }
  updateVisibleEnemies();
  if (swapPositionCooldown)
    --swapPositionCooldown;
  MEASURE(controller->makeMove(), "creature move time");
  CHECK(!inEquipChain) << "Someone forgot to finishEquipChain()";
  if (!hidden)
    viewObject.removeModifier(ViewObject::HIDDEN);
  unknownAttacker.clear();
  if (fireCreature && Random.roll(5))
    getSquare()->setOnFire(1);
  if (!getSquare()->isCovered())
    shineLight();
}

Square* Creature::getSquare() {
  return level->getSquare(position);
}

Square* Creature::getSquare(Vec2 direction) {
  return level->getSquare(position + direction);
}

const Square* Creature::getConstSquare() const {
  return getLevel()->getSquare(position);
}

const Square* Creature::getConstSquare(Vec2 direction) const {
  return getLevel()->getSquare(position + direction);
}

void Creature::wait() {
  Debug() << getTheName() << " waiting";
  bool keepHiding = hidden;
  spendTime(1);
  hidden = keepHiding;
}

const Equipment& Creature::getEquipment() const {
  return equipment;
}

Equipment& Creature::getEquipment() {
  return equipment;
}

vector<PItem> Creature::steal(const vector<Item*> items) {
  return equipment.removeItems(items);
}

Item* Creature::getAmmo() const {
  for (Item* item : equipment.getItems())
    if (item->getType() == ItemType::AMMO)
      return item;
  return nullptr;
}

const Level* Creature::getLevel() const {
  return level;
}

Level* Creature::getLevel() {
  return level;
}

Vec2 Creature::getPosition() const {
  return position;
}

void Creature::globalMessage(const string& playerCanSee, const string& cant) const {
  if (const Creature* player = level->getPlayer()) {
    if (player->canSee(this))
      player->privateMessage(playerCanSee);
    else
      player->privateMessage(cant);
  }
}

const vector<const Creature*>& Creature::getVisibleEnemies() const {
  return visibleEnemies;
}

void Creature::updateVisibleEnemies() {
  visibleEnemies.clear();
  for (const Creature* c : level->getAllCreatures()) 
    if (isEnemy(c) && (canSee(c)))
      visibleEnemies.push_back(c);
  for (const Creature* c : getUnknownAttacker())
    if (!contains(visibleEnemies, c))
      visibleEnemies.push_back(c);
}

vector<const Creature*> Creature::getVisibleCreatures() const {
  vector<const Creature*> res;
  for (Creature* c : level->getAllCreatures())
    if (canSee(c))
      res.push_back(c);
  for (const Creature* c : getUnknownAttacker())
    if (!contains(res, c))
      res.push_back(c);
  return res;
}

void Creature::addSkill(Skill* skill) {
  if (!skills.count(skill)) {
    skills.insert(skill);
    skill->onTeach(this);
    privateMessage(skill->getHelpText());
  }
}

bool Creature::hasSkill(Skill* skill) const {
  return skills.count(skill);
}

bool Creature::hasSkillToUseWeapon(const Item* it) const {
  return !it->isWieldedTwoHanded() || hasSkill(Skill::twoHandedWeapon);
}

vector<Item*> Creature::getPickUpOptions() const {
  if (!isHumanoid())
    return vector<Item*>();
  else
    return level->getSquare(getPosition())->getItems();
}

bool Creature::canPickUp(const vector<Item*>& items) const {
  if (!isHumanoid())
    return false;
  double weight = getInventoryWeight();
  for (Item* it : items)
    weight += it->getWeight();
  if (weight > 2 * getAttr(AttrType::INV_LIMIT)) {
    privateMessage("You are carrying too much to pick this up.");
    return false;
  }
  return true;
}

void Creature::pickUp(const vector<Item*>& items, bool spendT) {
  CHECK(canPickUp(items));
  Debug() << getTheName() << " pickup ";
  for (auto item : items) {
    equipment.addItem(level->getSquare(getPosition())->removeItem(item));
  }
  if (getInventoryWeight() > getAttr(AttrType::INV_LIMIT))
    privateMessage("You are overloaded.");
  EventListener::addPickupEvent(this, items);
  if (spendT)
    spendTime(1);
}

void Creature::drop(const vector<Item*>& items) {
  CHECK(isHumanoid());
  Debug() << getTheName() << " drop";
  for (auto item : items) {
    level->getSquare(getPosition())->dropItem(equipment.removeItem(item));
  }
  EventListener::addDropEvent(this, items);
  spendTime(1);
}

void Creature::drop(vector<PItem> items) {
  Debug() << getTheName() << " drop";
  getSquare()->dropItems(std::move(items));
}

void Creature::startEquipChain() {
  inEquipChain = true;
}

void Creature::finishEquipChain() {
  inEquipChain = false;
  if (numEquipActions > 0)
    spendTime(1);
  numEquipActions = 0;
}

bool Creature::canEquipIfEmptySlot(const Item* item) const {
  if (!isHumanoid())
    return false;
  if (numGoodArms() == 0) {
    privateMessage("You don't have hands!");
    return false;
  }
  if (!hasSkill(Skill::twoHandedWeapon) && item->isWieldedTwoHanded()) {
    privateMessage("You don't have the skill to use two-handed weapons.");
    return false;
  }
  if (!hasSkill(Skill::archery) && item->getType() == ItemType::RANGED_WEAPON) {
    privateMessage("You don't have the skill to shoot a bow.");
    return false;
  }
  if (numGoodArms() == 1 && item->isWieldedTwoHanded()) {
    privateMessage("You need two hands to wield " + item->getAName() + "!");
    return false;
  }
  return item->canEquip();
}

bool Creature::canEquip(const Item* item) const {
  return canEquipIfEmptySlot(item) && equipment.getItem(item->getEquipmentSlot()) == nullptr;
}

bool Creature::canUnequip(const Item* item) const {
  if (!equipment.isEquiped(item))
    return false;
  if (!isHumanoid())
    return false;
  if (numGoodArms() == 0) {
    privateMessage("You don't have hands!");
    return false;
  } else
    return true;
}

void Creature::equip(Item* item) {
  CHECK(canEquip(item));
  Debug() << getTheName() << " equip " << item->getName();
  EquipmentSlot slot = item->getEquipmentSlot();
  equipment.equip(item, slot);
  item->onEquip(this);
  EventListener::addEquipEvent(this, item);
  if (!inEquipChain)
    spendTime(1);
  else
    ++numEquipActions;
}

void Creature::unequip(Item* item) {
  CHECK(canUnequip(item));
  Debug() << getTheName() << " unequip";
  EquipmentSlot slot = item->getEquipmentSlot();
  CHECK(equipment.getItem(slot) == item) << "Item not equiped.";
  equipment.unequip(slot);
  item->onUnequip(this);
  if (!inEquipChain)
    spendTime(1);
  else
    ++numEquipActions;
}

bool Creature::canHeal(Vec2 direction) const {
  Creature* other = level->getSquare(position + direction)->getCreature();
  return healer && other && other->getHealth() < 1;
}

void Creature::heal(Vec2 direction) {
  CHECK(canHeal(direction));
  Creature* other = level->getSquare(position + direction)->getCreature();
  other->you(MsgType::ARE, "healed by " + getTheName());
  other->heal();
  spendTime(1);
}

bool Creature::canBumpInto(Vec2 direction) const {
  return level->getSquare(getPosition() + direction)->getCreature();
}

void Creature::bumpInto(Vec2 direction) {
  CHECK(canBumpInto(direction));
  level->getSquare(getPosition() + direction)->getCreature()->controller->onBump(this);
  spendTime(1);
}

void Creature::applySquare() {
  Debug() << getTheName() << " applying " << getSquare()->getName();;
  getSquare()->onApply(this);
  spendTime(1);
}

bool Creature::canHide() const {
  return skills.count(Skill::ambush) && getConstSquare()->canHide();
}

void Creature::hide() {
  knownHiding.clear();
  viewObject.setModifier(ViewObject::HIDDEN);
  for (const Creature* c : getLevel()->getAllCreatures())
    if (c->canSee(this) && c->isEnemy(this)) {
      knownHiding.insert(c);
      if (!isBlind())
        you(MsgType::CAN_SEE_HIDING, c->getTheName());
    }
  spendTime(1);
  hidden = true;
}

bool Creature::canChatTo(Vec2 direction) const {
  return getConstSquare(direction)->getCreature();
}

void Creature::chatTo(Vec2 direction) {
  CHECK(canChatTo(direction));
  Creature* c = getSquare(direction)->getCreature();
  c->onChat(this);
  spendTime(1);
}

void Creature::onChat(Creature* from) {
  if (isEnemy(from) && chatReactionHostile) {
    if (chatReactionHostile->front() == '\"')
      from->privateMessage(*chatReactionHostile);
    else
      from->privateMessage(getTheName() + " " + *chatReactionHostile);
  }
  if (!isEnemy(from) && chatReactionFriendly) {
    if (chatReactionFriendly->front() == '\"')
      from->privateMessage(*chatReactionFriendly);
    else
      from->privateMessage(getTheName() + " " + *chatReactionFriendly);
  }
}

void Creature::learnLocation(const Location* loc) {
  controller->learnLocation(loc);
}
void Creature::stealFrom(Vec2 direction, const vector<Item*>& items) {
  Creature* c = NOTNULL(getSquare(direction)->getCreature());
  equipment.addItems(c->steal(items));
}

bool Creature::isHidden() const {
  return hidden;
}

bool Creature::knowsHiding(const Creature* c) const {
  return knownHiding.count(c) == 1;
}

bool Creature::affects(LastingEffect effect) const {
  switch (effect) {
    case RAGE:
    case PANIC: return !isAffected(SLEEP);
    case BLIND: return !permanentlyBlind;
    case POISON: return !poisonResistant && !isNotLiving();
    case ENTANGLED: return !noBody;
    default: return true;
  }
}

void Creature::onAffected(LastingEffect effect) {
  switch (effect) {
    case PANIC:
      removeEffect(RAGE, false);
      you(MsgType::PANIC, "");
      break;
    case RAGE:
      removeEffect(PANIC, false);
      you(MsgType::RAGE, "");
      break;
    case HALLU: 
      if (!isBlind())
        privateMessage("The world explodes into colors!");
      break;
    case BLIND:
      you(MsgType::ARE, "blind!");
      viewObject.setModifier(ViewObject::BLIND);
      break;
    case INVISIBLE:
      if (!isBlind())
        you(MsgType::TURN_INVISIBLE, "");
      viewObject.setModifier(ViewObject::INVISIBLE);
      break;
    case POISON:
      you(MsgType::ARE, "poisoned");
      viewObject.setModifier(ViewObject::POISONED);
      break;
    case STR_BONUS: you(MsgType::FEEL, "stronger"); break;
    case DEX_BONUS: you(MsgType::FEEL, "more agile"); break;
    case SPEED: 
      you(MsgType::ARE, "moving faster");
      removeEffect(SLOWED, false);
      break;
    case SLOWED: 
      you(MsgType::ARE, "moving more slowly");
      removeEffect(SPEED, false);
      break;
    case ENTANGLED: you(MsgType::ARE, "entangled in a web"); break;
    case SLEEP: you(MsgType::FALL_ASLEEP, ""); break;
  }
}

void Creature::onRemoved(LastingEffect effect, bool msg) {
  switch (effect) {
    case POISON:
      if (msg)
        you(MsgType::ARE, "cured from poisoning");
      viewObject.removeModifier(ViewObject::POISONED);
      break;
    default: onTimedOut(effect, msg); break;
  }
}

void Creature::onTimedOut(LastingEffect effect, bool msg) {
  switch (effect) {
    case SLOWED: if (msg) you(MsgType::ARE, "moving faster again"); break;
    case SLEEP: if (msg) you(MsgType::WAKE_UP, ""); break;
    case SPEED: if (msg) you(MsgType::ARE, "moving more slowly again"); break;
    case STR_BONUS: if (msg) you(MsgType::ARE, "weaker again"); break;
    case DEX_BONUS: if (msg) you(MsgType::ARE, "less agile again"); break;
    case PANIC:
    case RAGE:
    case HALLU: if (msg) privateMessage("Your mind is clear again"); break;
    case ENTANGLED: if (msg) you(MsgType::BREAK_FREE, "the web"); break;
    case BLIND:
      if (msg) 
        you("can see again");
      viewObject.removeModifier(ViewObject::BLIND);
      break;
    case INVISIBLE:
      if (msg)
        you(MsgType::TURN_VISIBLE, "");
      viewObject.removeModifier(ViewObject::INVISIBLE);
      break;
    case POISON:
      if (msg)
        you(MsgType::ARE, "no longer poisoned");
      viewObject.removeModifier(ViewObject::POISONED);
      break;
  } 
}

void Creature::addEffect(LastingEffect effect, double time) {
  if (affects(effect)) {
    lastingEffects[effect] = getTime() + time;
    onAffected(effect);
  }
}

void Creature::removeEffect(LastingEffect effect, bool msg) {
  lastingEffects.erase(effect);
  onRemoved(effect, msg);
}

bool Creature::isAffected(LastingEffect effect) const {
  return lastingEffects.count(effect) && lastingEffects.at(effect) >= getTime();
}

bool Creature::isBlind() const {
  return isAffected(BLIND) || permanentlyBlind;
}

void Creature::makeStunned() {
  you(MsgType::ARE, "stunned");
  stunned = true;
}

int Creature::getAttrVal(AttrType type) const {
  switch (type) {
    case AttrType::SPEED: return *speed + getExpLevel() * 3;
    case AttrType::DEXTERITY: return *dexterity + getExpLevel() / 2;
    case AttrType::STRENGTH: return *strength + (getExpLevel() - 1) / 2;
    default: return 0;
  }
}

int attrBonus = 3;

int dexPenNoArm = 2;
int dexPenNoLeg = 10;
int dexPenNoWing = 3;

int strPenNoArm = 2;
int strPenNoLeg = 5;
int strPenNoWing = 2;

int Creature::getAttr(AttrType type) const {
  int def = getAttrVal(type);
  for (Item* item : equipment.getItems())
    if (equipment.isEquiped(item))
      def += item->getModifier(type);
  switch (type) {
    case AttrType::STRENGTH:
        def += tribe->getHandicap();
        if (health < 1)
          def *= 0.666 + health / 3;
        if (isAffected(SLEEP))
          def *= 0.66;
        if (isAffected(STR_BONUS))
          def += attrBonus;
        def -= (injuredArms + lostArms) * strPenNoArm + 
               (injuredLegs + lostLegs) * strPenNoLeg +
               (injuredWings + lostWings) * strPenNoWing;
        break;
    case AttrType::DEXTERITY:
        def += tribe->getHandicap();
        if (health < 1)
          def *= 0.666 + health / 3;
        if (isAffected(SLEEP))
          def = 0;
        if (isAffected(DEX_BONUS))
          def += attrBonus;
        def -= (injuredArms + lostArms) * dexPenNoArm + 
               (injuredLegs + lostLegs) * dexPenNoLeg +
               (injuredWings + lostWings) * dexPenNoWing;
        break;
    case AttrType::THROWN_DAMAGE: 
    case AttrType::DAMAGE: 
        def += getAttr(AttrType::STRENGTH);
        if (!getWeapon())
          def += barehandedDamage;
        if (isAffected(PANIC))
          def -= attrBonus;
        if (isAffected(RAGE))
          def += attrBonus;
        break;
    case AttrType::DEFENSE: 
        def += getAttr(AttrType::STRENGTH);
        if (isAffected(PANIC))
          def += attrBonus;
        if (isAffected(RAGE))
          def -= attrBonus;
        break;
    case AttrType::THROWN_TO_HIT: 
    case AttrType::TO_HIT: 
        def += getAttr(AttrType::DEXTERITY);
        break;
    case AttrType::SPEED: {
        double totWeight = getInventoryWeight();
        if (!carryAnything && totWeight > getAttr(AttrType::STRENGTH))
          def -= 20.0 * totWeight / def;
        if (isAffected(SLOWED))
          def /= 2;
        if (isAffected(SPEED))
          def *= 2;
        break;}
    case AttrType::INV_LIMIT:
        if (carryAnything)
          return 1000000;
        return getAttr(AttrType::STRENGTH) * 2;
 //   default:
 //       break;
  }
  return max(0, def);
}

int Creature::getPoints() const {
  return points;
}

void Creature::onKillEvent(const Creature* victim, const Creature* killer) {
  if (killer == this)
    points += victim->getDifficultyPoints();
}

double Creature::getInventoryWeight() const {
  double ret = 0;
  for (Item* item : getEquipment().getItems())
    ret += item->getWeight();
  return ret;
}

Tribe* Creature::getTribe() const {
  return tribe;
}

bool Creature::isFriend(const Creature* c) const {
  return !isEnemy(c);
}

pair<double, double> Creature::getStanding(const Creature* c) const {
  double bestWeight = 0;
  double standing = getTribe()->getStanding(c);
  if (contains(privateEnemies, c)) {
    standing = -1;
    bestWeight = 1;
  }
  for (EnemyCheck* enemyCheck : enemyChecks)
    if (enemyCheck->hasStanding(c) && enemyCheck->getWeight() > bestWeight) {
      standing = enemyCheck->getStanding(c);
      bestWeight = enemyCheck->getWeight();
    }
  return make_pair(standing, bestWeight);
}

void Creature::addEnemyCheck(EnemyCheck* c) {
  enemyChecks.push_back(c);
}

void Creature::removeEnemyCheck(EnemyCheck* c) {
  removeElement(enemyChecks, c);
}

bool Creature::isEnemy(const Creature* c) const {
  pair<double, double> myStanding = getStanding(c);
  pair<double, double> hisStanding = c->getStanding(this);
  double standing = 0;
  if (myStanding.second > hisStanding.second)
    standing = myStanding.first;
  if (myStanding.second < hisStanding.second)
    standing = hisStanding.first;
  if (myStanding.second == hisStanding.second)
    standing = min(myStanding.first, hisStanding.first);
  return c != this && standing < 0;
}

vector<Item*> Creature::getGold(int num) const {
  vector<Item*> ret;
  for (Item* item : equipment.getItems([](Item* it) { return it->getType() == ItemType::GOLD; })) {
    ret.push_back(item);
    if (ret.size() == num)
      return ret;
  }
  return ret;
}

void Creature::setPosition(Vec2 pos) {
  position = pos;
}

void Creature::setLevel(Level* l) {
  level = l;
}

double Creature::getTime() const {
  return time;
}

void Creature::setTime(double t) {
  time = t;
}

void Creature::tick(double realTime) {
  getDifficultyPoints();
  for (Item* item : equipment.getItems()) {
    item->tick(time, level, position);
    if (item->isDiscarded())
      equipment.removeItem(item);
  }
  for (auto elem : copyThis(lastingEffects))
    if (elem.second < realTime) {
      lastingEffects.erase(elem.first);
      onTimedOut(elem.first, true);
    }
  else if (isAffected(POISON)) {
    bleed(1.0 / 60);
    privateMessage("You feel poison flowing in your veins.");
  }
  double delta = realTime - lastTick;
  lastTick = realTime;
  updateViewObject();
  if (isNotLiving() && numLostOrInjuredBodyParts() >= 4) {
    you(MsgType::FALL_APART, "");
    die(lastAttacker);
    return;
  }
  if (health < 0.5) {
    health -= delta / 40;
    privateMessage("You are bleeding.");
  }
  if (health <= 0) {
    you(MsgType::DIE_OF, isAffected(POISON) ? "poisoning" : "bleeding");
    die(lastAttacker);
  }

}

BodyPart Creature::armOrWing() const {
  if (arms == 0)
    return BodyPart::WING;
  if (wings == 0)
    return BodyPart::ARM;
  return chooseRandom({ BodyPart::WING, BodyPart::ARM }, {1, 1});
}

BodyPart Creature::getBodyPart(AttackLevel attack) const {
  if (flyer)
    return chooseRandom({BodyPart::TORSO, BodyPart::HEAD, BodyPart::LEG, BodyPart::WING, BodyPart::ARM}, {1, 1, 1, 2, 1});
  switch (attack) {
    case AttackLevel::HIGH: 
       return BodyPart::HEAD;
    case AttackLevel::MIDDLE:
       if (size == CreatureSize::SMALL || size == CreatureSize::MEDIUM || collapsed)
         return BodyPart::HEAD;
       else
         return chooseRandom({BodyPart::TORSO, armOrWing()}, {1, 1});
    case AttackLevel::LOW:
       if (size == CreatureSize::SMALL || collapsed)
         return chooseRandom({BodyPart::TORSO, armOrWing(), BodyPart::HEAD, BodyPart::LEG}, {1, 1, 1, 1});
       if (size == CreatureSize::MEDIUM)
         return chooseRandom({BodyPart::TORSO, armOrWing(), BodyPart::LEG}, {1, 1, 3});
       else
         return BodyPart::LEG;
  }
  return BodyPart::ARM;
}

static string getAttackParam(AttackType type) {
  switch (type) {
    case AttackType::CUT: return "cut";
    case AttackType::STAB: return "stab";
    case AttackType::CRUSH: return "crush";
    case AttackType::PUNCH: return "punch";
    case AttackType::BITE: return "bite";
    case AttackType::HIT: return "hit";
    case AttackType::SHOOT: return "shot";
    case AttackType::SPELL: return "spell";
  }
  return "";
}

void Creature::injureLeg(bool drop) {
  if (legs == 0)
    return;
  if (drop) {
    Statistics::add(StatId::CHOPPED_LIMB);
    --legs;
    ++lostLegs;
    if (injuredLegs > legs)
      --injuredLegs;
  }
  else if (injuredLegs < legs)
    ++injuredLegs;
  if (!collapsed)
    you(MsgType::COLLAPSE, "");
  collapsed = true;
  if (drop) {
    getSquare()->dropItem(ItemFactory::corpse(*name + " leg", "bone", *weight / 8,
          isFood ? ItemType::FOOD : ItemType::CORPSE));
  }
}

void Creature::injureArm(bool dropArm) {
  if (dropArm) {
    Statistics::add(StatId::CHOPPED_LIMB);
    --arms;
    ++lostArms;
    if (injuredArms > arms)
      --injuredArms;
  }
  else if (injuredArms < arms)
    ++injuredArms;
  string itemName = isPlayer() ? ("your arm") : (*name + " arm");
  if (getWeapon()) {
    you(MsgType::DROP_WEAPON, getWeapon()->getName());
    level->getSquare(getPosition())->dropItem(equipment.removeItem(getWeapon()));
  }
  if (dropArm)
    getSquare()->dropItem(ItemFactory::corpse(*name + " arm", "bone", *weight / 12,
          isFood ? ItemType::FOOD : ItemType::CORPSE));
}

void Creature::injureWing(bool drop) {
  if (drop) {
    Statistics::add(StatId::CHOPPED_LIMB);
    --wings;
    ++lostWings;
    if (injuredWings > wings)
      --injuredWings;
  }
  else if (injuredWings < wings)
    ++injuredWings;
  if (flyer) {
    you(MsgType::FALL, getSquare()->getName());
    flyer = false;
  }
  if ((legs < 2 || injuredLegs > 0) && !collapsed) {
    collapsed = true;
  }
  string itemName = isPlayer() ? ("your wing") : (*name + " wing");
  if (drop)
    getSquare()->dropItem(ItemFactory::corpse(*name + " wing", "bone", *weight / 12,
          isFood ? ItemType::FOOD : ItemType::CORPSE));
}

void Creature::injureHead(bool drop) {
  if (drop) {
    Statistics::add(StatId::CHOPPED_HEAD);
    --heads;
    if (injuredHeads > heads)
      --injuredHeads;
  }
  else if (injuredHeads < heads)
    ++injuredHeads;
  if (drop)
    getSquare()->dropItem(ItemFactory::corpse(*name +" head", *name + " skull", *weight / 12,
          isFood ? ItemType::FOOD : ItemType::CORPSE));
}

static MsgType getAttackMsg(AttackType type, bool weapon, AttackLevel level) {
  if (weapon)
    return type == AttackType::STAB ? MsgType::THRUST_WEAPON : MsgType::SWING_WEAPON;
  switch (type) {
    case AttackType::BITE: return MsgType::BITE;
    case AttackType::PUNCH: return level == AttackLevel::LOW ? MsgType::KICK : MsgType::PUNCH;
    case AttackType::HIT: return MsgType::HIT;
    default: FAIL << "Unhandled barehanded attack: " << int(type);
  }
  return MsgType(0);
}

void Creature::attack(const Creature* c1, bool spend) {
  Creature* c = const_cast<Creature*>(c1);
  int toHitVariance = 9;
  int damageVariance = 9;
  CHECK((c->getPosition() - getPosition()).length8() == 1)
      << "Bad attack direction " << c->getPosition() - getPosition();
  CHECK(canAttack(c));
  Debug() << getTheName() << " attacking " << c->getName();
  auto rToHit = [=] () { return Random.getRandom(GET_ID(getUniqueId()), -toHitVariance, toHitVariance); };
  auto rDamage = [=] () { return Random.getRandom(GET_ID(getUniqueId()), -damageVariance, damageVariance); };
  int toHit = rToHit() + rToHit() + getAttr(AttrType::TO_HIT);
  int damage = rDamage() + rDamage() + getAttr(AttrType::DAMAGE);
  bool backstab = false;
  string enemyName = getLevel()->playerCanSee(c) ? c->getTheName() : "something";
  if (c->isPlayer())
    enemyName = "";
  if (!c->canSee(this) && canSee(c)) {
 //   if (getWeapon() && getWeapon()->getAttackType() == AttackType::STAB) {
      damage += 10;
      backstab = true;
 //   }
    you(MsgType::ATTACK_SURPRISE, enemyName);
  }
  Attack attack(this, getRandomAttackLevel(), getAttackType(), toHit, damage, backstab, attackEffect);
  if (!c->dodgeAttack(attack)) {
    if (getWeapon()) {
      you(getAttackMsg(attack.getType(), true, attack.getLevel()), getWeapon()->getName());
      if (!canSee(c))
        privateMessage("You hit something.");
    }
    else {
      you(getAttackMsg(attack.getType(), false, attack.getLevel()), enemyName);
    }
    c->takeDamage(attack);
    if (c->isDead())
      increaseExpLevel(c->getDifficultyPoints() / 400);
  }
  else
    you(MsgType::MISS_ATTACK, enemyName);
  if (spend)
    spendTime(1);
}

bool Creature::dodgeAttack(const Attack& attack) {
  Debug() << getTheName() << " dodging " << attack.getAttacker()->getName() << " to hit " << attack.getToHit() << " dodge " << getAttr(AttrType::TO_HIT);
  if (const Creature* c = attack.getAttacker()) {
    if (!canSee(c))
      unknownAttacker.push_back(c);
    EventListener::addAttackEvent(this, c);
    if (!contains(privateEnemies, c) && c->getTribe() != tribe)
      privateEnemies.push_back(c);
  }
  return canSee(attack.getAttacker()) && attack.getToHit() <= getAttr(AttrType::TO_HIT);
}

bool Creature::takeDamage(const Attack& attack) {
  if (isAffected(SLEEP))
    removeEffect(SLEEP);
  if (const Creature* c = attack.getAttacker())
    if (!contains(privateEnemies, c) && c->getTribe() != tribe)
      privateEnemies.push_back(c);
  int defense = getAttr(AttrType::DEFENSE);
  Debug() << getTheName() << " attacked by " << attack.getAttacker()->getName() << " damage " << attack.getStrength() << " defense " << defense;
  if (passiveAttack && attack.getAttacker() && attack.getAttacker()->getPosition().dist8(position) == 1) {
    Creature* other = const_cast<Creature*>(attack.getAttacker());
    Effect::applyToCreature(other, *passiveAttack, EffectStrength::NORMAL);
    other->lastAttacker = this;
  }
  if (attack.getStrength() > defense) {
    if (auto effect = attack.getEffect())
      Effect::applyToCreature(this, *effect, EffectStrength::NORMAL);
    lastAttacker = attack.getAttacker();
    double dam = (defense == 0) ? 1 : double(attack.getStrength() - defense) / defense;
    dam *= damageMultiplier;
    if (!isNotLiving())
      bleed(dam);
    if (!noBody) {
      if (attack.getType() != AttackType::SPELL) {
        BodyPart part = attack.inTheBack() ? BodyPart::BACK : getBodyPart(attack.getLevel());
        switch (part) {
          case BodyPart::BACK:
            youHit(part, attack.getType());
            break;
          case BodyPart::WING:
            if (dam >= 0.3 && wings > injuredWings) {
              youHit(BodyPart::WING, attack.getType()); 
              injureWing(attack.getType() == AttackType::CUT || attack.getType() == AttackType::BITE);
              if (health <= 0)
                health = 0.01;
              return false;
            }
          case BodyPart::ARM:
            if (dam >= 0.5 && arms > injuredArms) {
              youHit(BodyPart::ARM, attack.getType()); 
              injureArm(attack.getType() == AttackType::CUT || attack.getType() == AttackType::BITE);
              if (health <= 0)
                health = 0.01;
              return false;
            }
          case BodyPart::LEG:
            if (dam >= 0.8 && legs > injuredLegs) {
              youHit(BodyPart::LEG, attack.getType()); 
              injureLeg(attack.getType() == AttackType::CUT || attack.getType() == AttackType::BITE);
              if (health <= 0)
                health = 0.01;
              return false;
            }
          case BodyPart::HEAD:
            if (dam >= 0.8 && heads > injuredHeads) {
              youHit(BodyPart::HEAD, attack.getType()); 
              injureHead(attack.getType() == AttackType::CUT || attack.getType() == AttackType::BITE);
              if (!isNotLiving()) {
                you(MsgType::DIE, "");
                die(attack.getAttacker());
              }
              return true;
            }
          case BodyPart::TORSO:
            if (dam >= 1.5) {
              youHit(BodyPart::TORSO, attack.getType());
              if (!isNotLiving())
                you(MsgType::DIE, "");
              die(attack.getAttacker());
              return true;
            }
            break;
        }
      }
    } else {
      you(MsgType::TURN, "whisp of smoke");
      die(attack.getAttacker());
      return true;
    }
    if (health <= 0) {
      you(MsgType::ARE, "critically wounded");
      you(MsgType::DIE, "");
      die(attack.getAttacker());
      return true;
    } else
    if (health < 0.5)
      you(MsgType::ARE, "critically wounded");
    else
      you(MsgType::ARE, "wounded");
  } else
    you(MsgType::GET_HIT_NODAMAGE, getAttackParam(attack.getType()));
  const Creature* c = attack.getAttacker();
  return false;
}

void Creature::updateViewObject() {
  viewObject.setDefense(getAttr(AttrType::DEFENSE));
  viewObject.setAttack(getAttr(AttrType::DAMAGE));
  if (const Creature* c = getLevel()->getPlayer()) {
    if (isEnemy(c))
      viewObject.setEnemyStatus(ViewObject::HOSTILE);
    else
      viewObject.setEnemyStatus(ViewObject::FRIENDLY);
  }
  viewObject.setBleeding(1 - health);
}

double Creature::getHealth() const {
  return health;
}

double Creature::getWeight() const {
  return *weight;
}

string sizeStr(CreatureSize s) {
  switch (s) {
    case CreatureSize::SMALL: return "small";
    case CreatureSize::MEDIUM: return "medium";
    case CreatureSize::LARGE: return "large";
    case CreatureSize::HUGE: return "huge";
  }
  return 0;
}

static string adjectives(CreatureSize s, bool undead, bool notLiving, bool noBody) {
  vector<string> ret {sizeStr(s)};
  if (notLiving)
    ret.push_back("non-living");
  if (undead)
    ret.push_back("undead");
  if (noBody)
    ret.push_back("body-less");
  return combine(ret);
}

string limbsStr(int arms, int legs, int wings) {
  vector<string> ret;
  if (arms)
    ret.push_back("arms");
  if (legs)
    ret.push_back("legs");
  if (wings)
    ret.push_back("wings");
  if (ret.size() > 0)
    return " with " + combine(ret);
  else
    return "";
}

string attrStr(bool strong, bool agile, bool fast) {
  vector<string> good;
  vector<string> bad;
  if (strong)
    good.push_back("strong");
  else
    bad.push_back("weak");
  if (agile)
    good.push_back("agile");
  else
    bad.push_back("clumsy");
  if (fast)
    good.push_back("fast");
  else
    bad.push_back("slow");
  string p1 = combine(good);
  string p2 = combine(bad);
  if (p1.size() > 0 && p2.size() > 0)
    p1.append(", but ");
  p1.append(p2);
  return p1;
}

bool Creature::isSpecialMonster() const {
  return specialMonster;
}

string Creature::getDescription() const {
  string weapon;
  /*if (Item* item = getEquipment().getItem(EquipmentSlot::WEAPON))
    weapon = " It's wielding " + item->getAName() + ".";
  else
  if (Item* item = getEquipment().getItem(EquipmentSlot::RANGED_WEAPON))
    weapon = " It's wielding " + item->getAName() + ".";*/
  string attack;
  if (attackEffect) {
    switch (*attackEffect) {
      case EffectType::POISON: attack = "poison"; break;
      case EffectType::FIRE: attack = "fire"; break;
      default: FAIL << "Unhandled monster attack " << int(*attackEffect);
    }
    attack = " It has a " + attack + " attack.";
  }
  return getTheName() + " is a " + adjectives(*size, undead, notLiving, noBody) +
      (isHumanoid() ? " humanoid creature" : " beast") +
      (!isHumanoid() ? limbsStr(arms, legs, wings) : (wings ? " with wings" : "")) + ". " +
     "It is " + attrStr(*strength > 16, *dexterity > 16, *speed > 100) + "." + weapon + attack;
}

void Creature::setSpeed(double value) {
  speed = value;
}

double Creature::getSpeed() const {
  return *speed;
}
  
CreatureSize Creature::getSize() const {
  return *size;
}

void Creature::heal(double amount, bool replaceLimbs) {
  Debug() << getTheName() << " heal";
  if (health < 1) {
    health = min(1., health + amount);
    if (health >= 0.5) {
      if (injuredArms > 0) {
        you(MsgType::YOUR, string(injuredArms > 1 ? "arms are" : "arm is") + " in better shape");
        injuredArms = 0;
      }
      if (lostArms > 0 && replaceLimbs) {
        you(MsgType::YOUR, string(lostArms > 1 ? "arms grow back!" : "arm grows back!"));
        arms += lostArms;
        lostArms = 0;
      }
      if (injuredWings > 0) {
        you(MsgType::YOUR, string(injuredArms > 1 ? "wings are" : "wing is") + " in better shape");
        injuredWings = 0;
      }
      if (lostWings > 0 && replaceLimbs) {
        you(MsgType::YOUR, string(lostArms > 1 ? "wings grow back!" : "wing grows back!"));
        wings += lostWings;
        lostWings = 0;
        flyer = true;
      }
     if (injuredLegs > 0) {
        you(MsgType::YOUR, string(injuredLegs > 1 ? "legs are" : "leg is") + " in better shape");
        injuredLegs = 0;
        if (legs == 2 && collapsed) {
          collapsed = false;
          you(MsgType::STAND_UP, "");
        }
      }
      if (lostLegs > 0 && replaceLimbs) {
        you(MsgType::YOUR, string(lostLegs > 1 ? "legs grow back!" : "leg grows back!"));
        legs += lostLegs;
        lostLegs = 0;
      }
    }
    if (health == 1) {
      you(MsgType::BLEEDING_STOPS, "");
      health = 1;
      lastAttacker = nullptr;
    }
    updateViewObject();
  }
}

void Creature::bleed(double severity) {
  updateViewObject();
  health -= severity;
  updateViewObject();
  Debug() << getTheName() << " health " << health;
}

void Creature::setOnFire(double amount) {
  if (!isFireResistant()) {
    you(MsgType::ARE, "burnt by the fire");
    bleed(6. * amount / double(getAttr(AttrType::STRENGTH)));
  }
}

void Creature::poisonWithGas(double amount) {
  if (!poisonResistant && breathing && !isNotLiving()) {
    you(MsgType::ARE, "poisoned by the gas");
    bleed(amount / double(getAttr(AttrType::STRENGTH)));
  }
}

void Creature::shineLight() {
  if (undead) {
    if (Random.roll(10)) {
      you(MsgType::YOUR, "body crumbles to dust");
      die(nullptr);
    } else
      you(MsgType::ARE, "burnt by the sun");
  }
}

void Creature::setHeld(const Creature* c) {
  holding = c;
}

bool Creature::isHeld() const {
  return holding != nullptr;
}

bool Creature::canSleep() const {
  return !noSleep;
}

void Creature::take(vector<PItem> items) {
  for (PItem& elem : items)
    take(std::move(elem));
}

void Creature::take(PItem item) {
 /* item->identify();
  Debug() << (specialMonster ? "special monster " : "") + getTheName() << " takes " << item->getNameAndModifiers();*/
  if (item->isWieldedTwoHanded())
    addSkill(Skill::twoHandedWeapon);
  if (item->getType() == ItemType::RANGED_WEAPON)
    addSkill(Skill::archery);
  Item* ref = item.get();
  equipment.addItem(std::move(item));
  if (canEquip(ref))
    equip(ref);
}

void Creature::dropCorpse() {
  getSquare()->dropItem(ItemFactory::corpse(*name + " corpse", *name + " skeleton", *weight,
        isFood ? ItemType::FOOD : ItemType::CORPSE, {true, heads > 0, false}));
}

void Creature::die(const Creature* attacker, bool dropInventory) {
  Debug() << getTheName() << " dies. Killed by " << (attacker ? attacker->getName() : "");
  controller->onKilled(attacker);
  if (attacker)
    attacker->kills.push_back(this);
  if (dropInventory)
    for (PItem& item : equipment.removeAllItems()) {
      level->getSquare(position)->dropItem(std::move(item));
    }
  dead = true;
  if (dropInventory && !noBody)
    dropCorpse();
  level->killCreature(this);
  EventListener::addKillEvent(this, attacker);
  if (innocent)
    Statistics::add(StatId::INNOCENT_KILLED);
  Statistics::add(StatId::DEATH);
}

bool Creature::canFlyAway() const {
  return canFly() && !getConstSquare()->isCovered();
}

void Creature::flyAway() {
  Debug() << getTheName() << " fly away";
  CHECK(canFlyAway());
  globalMessage(getTheName() + " flies away.");
  dead = true;
  level->killCreature(this);
}

void Creature::give(const Creature* whom, vector<Item*> items) {
  getLevel()->getSquare(whom->getPosition())->getCreature()->takeItems(equipment.removeItems(items), this);
}

bool Creature::canFire(Vec2 direction) const {
  CHECK(direction.length8() == 1);
  if (!getEquipment().getItem(EquipmentSlot::RANGED_WEAPON))
    return false;
  if (!hasSkill(Skill::archery)) {
    privateMessage("You don't have the skill to shoot a bow.");
    return false;
  }
  if (numGoodArms() < 2) {
    privateMessage("You need two hands to shoot a bow.");
    return false;
  }
  if (!getAmmo()) {
    privateMessage("Out of ammunition");
    return false;
  }
  return true;
}

void Creature::fire(Vec2 direction) {
  CHECK(canFire(direction));
  PItem ammo = equipment.removeItem(NOTNULL(getAmmo()));
  RangedWeapon* weapon = NOTNULL(dynamic_cast<RangedWeapon*>(getEquipment().getItem(EquipmentSlot::RANGED_WEAPON)));
  weapon->fire(this, level, std::move(ammo), direction);
  spendTime(1);
}

void Creature::squash(Vec2 direction) {
  if (canDestroy(direction))
    getSquare(direction)->destroy(getAttr(AttrType::DAMAGE));
  if (Creature* c = getSquare(direction)->getCreature()) {
    c->you(MsgType::KILLED_BY, getTheName());
    c->die(this);
  }
}

void Creature::construct(Vec2 direction, SquareType type) {
  getSquare(direction)->construct(type);
  spendTime(1);
}

bool Creature::canConstruct(Vec2 direction, SquareType type) const {
  return getConstSquare(direction)->canConstruct(type) && canConstruct(type);
}

bool Creature::canConstruct(SquareType type) const {
  return hasSkill(Skill::construction);
}

void Creature::eat(Item* item) {
  getSquare()->removeItem(item);
  spendTime(3);
}

bool Creature::canDestroy(Vec2 direction) const {
  return getConstSquare(direction)->canDestroy();
}

void Creature::destroy(Vec2 direction) {
  getSquare(direction)->destroy(getAttr(AttrType::DAMAGE));
  spendTime(1);
}

bool Creature::canAttack(const Creature* c) const {
  Vec2 direction = c->getPosition() - getPosition();
  return direction.length8() == 1;
}

AttackLevel Creature::getRandomAttackLevel() const {
  if (isHumanoid() && injuredArms == arms)
    return AttackLevel::LOW;
  switch (*size) {
    case CreatureSize::SMALL: return AttackLevel::LOW;
    case CreatureSize::MEDIUM: return chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE}, {1,1});
    case CreatureSize::LARGE: return chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH},{1,2,2});
    case CreatureSize::HUGE: return chooseRandom({AttackLevel::MIDDLE, AttackLevel::HIGH}, {1,3});
  }
  return AttackLevel::LOW;
}

Item* Creature::getWeapon() const {
  return equipment.getItem(EquipmentSlot::WEAPON);
}

AttackType Creature::getAttackType() const {
  if (getWeapon())
    return getWeapon()->getAttackType();
  else if (barehandedAttack)
    return *barehandedAttack;
  else
    return isHumanoid() ? AttackType::PUNCH : AttackType::BITE;
}

void Creature::applyItem(Item* item) {
  Debug() << getTheName() << " applying " << item->getAName();
  CHECK(canApplyItem(item));
  double time = item->getApplyTime();
  item->apply(this, level);
  if (item->isDiscarded()) {
    equipment.removeItem(item);
  }
  spendTime(time);
}

bool Creature::canApplyItem(const Item* item) const {
  return contains({ItemType::TOOL, ItemType::POTION, ItemType::FOOD, ItemType::BOOK, ItemType::SCROLL},
      item->getType()) && isHumanoid() && numGoodArms() > 0;
}

bool Creature::canThrowItem(Item* item) {
  if (injuredArms == arms || !isHumanoid()) {
    privateMessage("You can't throw anything!");
    return false;
  } else if (item->getWeight() > 20) {
    privateMessage(item->getTheName() + " is too heavy!");
    return false;
  }
    return true;
}

void Creature::throwItem(Item* item, Vec2 direction) {
  Debug() << getTheName() << " throwing " << item->getAName();
  CHECK(canThrowItem(item));
  int dist = 0;
  int toHitVariance = 10;
  int attackVariance = 7;
  int str = getAttr(AttrType::STRENGTH);
  if (item->getWeight() <= 0.5)
    dist = 10 * str / 15;
  else if (item->getWeight() <= 5)
    dist = 5 * str / 15;
  else if (item->getWeight() <= 20)
    dist = 2 * str / 15;
  else 
    FAIL << "Item too heavy.";
  int toHit = Random.getRandom(GET_ID(getUniqueId()), -toHitVariance, toHitVariance) +
      getAttr(AttrType::THROWN_TO_HIT) + item->getModifier(AttrType::THROWN_TO_HIT);
  int damage = Random.getRandom(GET_ID(getUniqueId()), -attackVariance, attackVariance) +
      getAttr(AttrType::THROWN_DAMAGE) + item->getModifier(AttrType::THROWN_DAMAGE);
  if (hasSkill(Skill::knifeThrowing) && item->getAttackType() == AttackType::STAB) {
    damage += 7;
    toHit += 4;
  }
  Attack attack(this, getRandomAttackLevel(), item->getAttackType(), toHit, damage, false, Nothing());
  level->throwItem(equipment.removeItem(item), attack, dist, getPosition(), direction);
  spendTime(1);
}

const ViewObject& Creature::getViewObject() const {
  return viewObject;
}

/*void Creature::setViewObject(const ViewObject& obj) {
  viewObject = obj;
}*/

bool Creature::canSee(const Creature* c) const {
  for (Vision* v : visions)
    if (v->canSee(this, c))
      return true;
  return !isBlind() && !c->isAffected(INVISIBLE) &&
         (!c->isHidden() || c->knowsHiding(this)) && 
         getLevel()->canSee(position, c->getPosition());
}

bool Creature::canSee(Vec2 pos) const {
  return !isBlind() && 
      getLevel()->canSee(position, pos);
}
 
bool Creature::isPlayer() const {
  return controller->isPlayer();
}
  
string Creature::getTheName() const {
  if (islower((*name)[0]))
    return "the " + *name;
  return *name;
}
 
string Creature::getAName() const {
  if (islower((*name)[0]))
    return "a " + *name;
  return *name;
}

Optional<string> Creature::getFirstName() const {
  return firstName;
}

string Creature::getName() const {
  return *name;
}

string Creature::getSpeciesName() const {
  if (speciesName)
    return *speciesName;
  else
    return *name;
}

bool Creature::isHumanoid() const {
  return *humanoid;
}

bool Creature::isAnimal() const {
  return animal;
}

bool Creature::isStationary() const {
  return stationary;
}

void Creature::setStationary() {
  stationary = true;
}

bool Creature::isInvincible() const {
  return invincible;
}

bool Creature::isUndead() const {
  return undead;
}

bool Creature::isNotLiving() const {
  return undead || notLiving;
}

bool Creature::hasBrain() const {
  return brain;
}

bool Creature::canSwim() const {
  return contains(skills, Skill::swimming);
}

bool Creature::canFly() const {
  return flyer;
}

bool Creature::canWalk() const {
  return walker;
}

int Creature::numArms() const {
  return arms;
}

int Creature::numLegs() const {
  return legs;
}

int Creature::numWings() const {
  return wings;
}

int Creature::numHeads() const {
  return heads;
}

bool Creature::lostLimbs() const {
  return lostWings > 0 || lostArms > 0 || lostLegs > 0;
}

int Creature::numLostOrInjuredBodyParts() const {
  return lostWings + injuredWings + lostArms + injuredArms + lostLegs + injuredLegs + lostHeads + injuredHeads;
}

int Creature::numGoodArms() const {
  return arms - injuredArms;
}

int Creature::numGoodLegs() const {
  return legs - injuredLegs;
}

int Creature::numGoodHeads() const {
  return heads - injuredHeads;
}

double Creature::getCourage() const {
  return courage;
}

Gender Creature::getGender() const {
  return gender;
}

void Creature::increaseExpLevel(double amount) {
  if (expLevel < maxLevel && increaseExperience) {
    expLevel += amount;
 //   viewObject.setSizeIncrease(0.3);
    if (skillGain.count(getExpLevel()) && isHumanoid()) {
      you(MsgType::ARE, "more experienced");
      addSkill(skillGain.at(getExpLevel()));
      skillGain.erase(getExpLevel());
    }
  }
}

int Creature::getExpLevel() const {
  return expLevel;
}

int Creature::getDifficultyPoints() const {
  difficultyPoints = max<double>(difficultyPoints,
      getAttr(AttrType::DEFENSE) + getAttr(AttrType::TO_HIT) + getAttr(AttrType::DAMAGE)
      + getAttr(AttrType::SPEED) / 10);
  return difficultyPoints;
}

Optional<Vec2> Creature::getMoveTowards(Vec2 pos, bool avoidEnemies) {
  return getMoveTowards(pos, false, avoidEnemies);
}

Optional<Vec2> Creature::getMoveTowards(Vec2 pos, bool away, bool avoidEnemies) {
  Debug() << "" << getPosition() << (away ? "Moving away from" : " Moving toward ") << pos;
  bool newPath = false;
  bool targetChanged = shortestPath && shortestPath->getTarget().dist8(pos) > getPosition().dist8(pos) / 10;
  if (!shortestPath || targetChanged || shortestPath->isReversed() != away) {
    newPath = true;
    if (!away)
      shortestPath = ShortestPath(getLevel(), this, pos, getPosition());
    else
      shortestPath = ShortestPath(getLevel(), this, pos, getPosition(), -1.5);
  }
  CHECK(shortestPath);
  if (shortestPath->isReachable(getPosition())) {
    Vec2 pos2 = shortestPath->getNextMove(getPosition());
    if (canMove(pos2 - getPosition())) {
      return pos2 - getPosition();
    }
  }
  if (newPath)
    return Nothing();
  Debug() << "Reconstructing shortest path.";
  if (!away)
    shortestPath = ShortestPath(getLevel(), this, pos, getPosition());
  else
    shortestPath = ShortestPath(getLevel(), this, pos, getPosition(), -1.5);
  if (shortestPath->isReachable(getPosition())) {
    Vec2 pos2 = shortestPath->getNextMove(getPosition());
    if (canMove(pos2 - getPosition())) {
      return pos2 - getPosition();
    } else
      return Nothing();
  } else {
    Debug() << "Cannot move toward " << pos;
    return Nothing();
  }
}

Optional<Vec2> Creature::getMoveAway(Vec2 pos, bool pathfinding) {
  if ((pos - getPosition()).length8() <= 5 && pathfinding) {
    Optional<Vec2> move = getMoveTowards(pos, true, false);
    if (move)
      return move;
  }
  pair<Vec2, Vec2> dirs = (getPosition() - pos).approxL1();
  vector<Vec2> moves;
  if (canMove(dirs.first))
    moves.push_back(dirs.first);
  if (canMove(dirs.second))
    moves.push_back(dirs.second);
  if (moves.size() > 0)
    return moves[Random.getRandom(moves.size())];
  return Nothing();
}

bool Creature::atTarget() const {
  return shortestPath && getPosition() == shortestPath->getTarget();
}

void Creature::youHit(BodyPart part, AttackType type) const {
  switch (part) {
    case BodyPart::BACK:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::ARE, "shot in the back!"); break;
          case AttackType::BITE: you(MsgType::ARE, "bitten in the neck!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "throat is cut!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "spine is crushed!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "neck is broken!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the back of the head!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the " + 
                                     chooseRandom<string>({"back", "neck"})); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::HEAD: 
        switch (type) {
          case AttackType::SHOOT: you(MsgType::ARE, "shot in the " +
                                      chooseRandom<string>({"eye", "neck", "forehead"}) + "!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "head is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "head is chopped off!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "skull is shattered!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "neck is broken!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the head!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the eye!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::TORSO:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the heart!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "internal organs are ripped out!"); break;
          case AttackType::CUT: you(MsgType::ARE, "cut in half!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the " +
                                     chooseRandom<string>({"stomach", "heart"}, {1, 1}) + "!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "ribs and internal organs are crushed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the chest!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "stomach receives a deadly blow!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::ARM:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the arm!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "arm is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "arm is chopped off!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the arm!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "arm is smashed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the arm!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "arm is broken!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::WING:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the wing!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "wing is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "wing is chopped off!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the wing!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "wing is smashed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the wing!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "wing is broken!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::LEG:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the leg!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "leg is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "leg is cut off!"); break;
          case AttackType::STAB: you(MsgType::YOUR, "stabbed in the leg!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "knee is crushed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the leg!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "leg is broken!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
  }
}

vector<const Creature*> Creature::getUnknownAttacker() const {
  return unknownAttacker;
}

string Creature::getNameAndTitle() const {
  if (firstName)
    return *firstName + " the " + getName();
  else
    return getTheName();
}

void Creature::refreshGameInfo(View::GameInfo& gameInfo) const {
  gameInfo.infoType = View::GameInfo::InfoType::PLAYER;
  View::GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  if (firstName) {
    info.playerName = *firstName;
    info.title = *name;
  } else {
    info.playerName = "";
    info.title = *name;
  }
  info.adjectives.clear();
  info.possessed = canPopController();
  info.spellcaster = !spells.empty();
  if (isBlind())
    info.adjectives.push_back("blind");
  if (isAffected(INVISIBLE))
    info.adjectives.push_back("invisible");
  if (numArms() == 1)
    info.adjectives.push_back("one-armed");
  if (numArms() == 0)
    info.adjectives.push_back("armless");
  if (numLegs() == 1)
    info.adjectives.push_back("one-legged");
  if (numLegs() == 0)
    info.adjectives.push_back("legless");
  if (isAffected(HALLU))
    info.adjectives.push_back("tripped");
  Item* weapon = getEquipment().getItem(EquipmentSlot::WEAPON);
  info.weaponName = weapon ? weapon->getAName() : "";
  const Location* location = getLevel()->getLocation(getPosition());
  info.levelName = location && location->hasName() 
    ? capitalFirst(location->getName()) : getLevel()->getName();
  info.speed = getAttr(AttrType::SPEED);
  info.speedBonus = isAffected(SPEED) ? 1 : isAffected(SLOWED) ? -1 : 0;
  info.defense = getAttr(AttrType::DEFENSE);
  info.defBonus = isAffected(RAGE) ? -1 : isAffected(PANIC) ? 1 : 0;
  info.attack = getAttr(AttrType::DAMAGE);
  info.attBonus = isAffected(RAGE) ? 1 : isAffected(PANIC) ? -1 : 0;
  info.strength = getAttr(AttrType::STRENGTH);
  info.strBonus = isAffected(STR_BONUS);
  info.dexterity = getAttr(AttrType::DEXTERITY);
  info.dexBonus = isAffected(DEX_BONUS);
  info.time = getTime();
  info.numGold = getGold(100000000).size();
  info.elfStanding = Tribes::get(TribeId::ELVEN)->getStanding(this);
  info.dwarfStanding = Tribes::get(TribeId::DWARVEN)->getStanding(this);
  info.goblinStanding = Tribes::get(TribeId::GOBLIN)->getStanding(this);
  info.effects.clear();
  for (LastingEffect effect : getKeys(lastingEffects))
    if (isAffected(effect))
      switch (effect) {
        case POISON: info.effects.push_back({"poisoned", true}); break;
        case SLEEP: info.effects.push_back({"sleeping", true}); break;
        case ENTANGLED: info.effects.push_back({"entangled", true}); break;
        default: break;
      }
}

