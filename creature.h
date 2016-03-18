/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _CREATURE_H
#define _CREATURE_H

#include "enums.h"
#include "unique_entity.h"
#include "creature_action.h"
#include "renderable.h"
#include "movement_type.h"
#include "position.h"
#include "event_generator.h"

class Skill;
class Level;
class Tribe;
class ViewObject;
class Attack;
class Controller;
class ControllerFactory;
class PlayerMessage;
class CreatureVision;
class SquareType;
class Location;
class ShortestPath;
class LevelShortestPath;
class Equipment;
class Spell;
class CreatureAttributes;
class MinionTaskMap;
class EntityName;
class Gender;
class SpellMap;
class Sound;
class Game;
class CreatureListener;

class Creature : public Renderable, public UniqueEntity<Creature> {
  public:
  Creature(TribeId, const CreatureAttributes&, const ControllerFactory&);
  Creature(const ViewObject&, TribeId, const CreatureAttributes&, const ControllerFactory&);
  virtual ~Creature();

  static vector<vector<Creature*>> stack(const vector<Creature*>&);

  const ViewObject& getViewObjectFor(const Tribe* observer) const;
  void makeMove();
  double getLocalTime() const;
  double getGlobalTime() const;
  void setLocalTime(double t);
  Level* getLevel() const;
  Game* getGame() const;
  vector<const Creature*> getVisibleEnemies() const;
  vector<Position> getVisibleTiles() const;
  void setPosition(Position);
  Position getPosition() const;
  bool dodgeAttack(const Attack&);
  bool takeDamage(const Attack&);
  void heal(double amount = 1, bool replaceLimbs = false);
  double getHealth() const;
  double getMorale() const;
  void addMorale(double);
  DEF_UNIQUE_PTR(MoraleOverride);
  class MoraleOverride {
    public:
    virtual optional<double> getMorale() = 0;
    virtual ~MoraleOverride() {}
    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };
  void addMoraleOverride(PMoraleOverride);

  double getWeight() const;
  bool canSleep() const;
  void take(PItem item);
  void take(vector<PItem> item);
  const Equipment& getEquipment() const;
  Equipment& getEquipment();
  vector<PItem> steal(const vector<Item*> items);
  bool canSee(const Creature*) const;
  bool canSee(Position) const;
  bool canSee(Vec2) const;
  bool isEnemy(const Creature*) const;
  void tick();

  const EntityName& getName() const;
  string getSpeciesName() const;
  string getNameAndTitle() const;
  optional<string> getFirstName() const;
  void setFirstName(const string&);
  string getGroupName(int count) const;
  int getModifier(ModifierType) const;
  int getAttr(AttrType) const;
  static string getAttrName(AttrType);
  static string getModifierName(ModifierType);

  int getPoints() const;
  VisionId getVision() const;

  const Tribe* getTribe() const;
  Tribe* getTribe();
  TribeId getTribeId() const;
  void setTribe(TribeId);
  bool isFriend(const Creature*) const;
  int getDebt(const Creature* debtor) const;
  vector<Item*> getGold(int num) const;

  void takeItems(vector<PItem> items, const Creature* from);
  bool canTakeItems(const vector<Item*>& items) const;

  void youHit(BodyPart part, AttackType type) const;

  void dropCorpse();
  virtual vector<PItem> getCorpse();
  
  void monsterMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const;
  void monsterMessage(const PlayerMessage& playerCanSee) const;
  void globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const;
  void globalMessage(const PlayerMessage& playerCanSee) const;

  bool isDead() const;
  bool isBlind() const;
  bool isBleeding() const;
  const Creature* getLastAttacker() const;
  optional<string> getDeathReason() const;
  double getDeathTime() const;
  vector<const Creature*> getKills() const;
  bool isHumanoid() const;
  bool isAnimal() const;
  bool isStationary() const;
  void setStationary();
  bool isInvincible() const;
  bool isUndead() const;
  bool hasBrain() const;
  bool isNotLiving() const;
  bool isCorporal() const;
  bool isWorshipped() const;
  bool dontChase() const;
  optional<SpawnType> getSpawnType() const;
  int getRecruitmentCost() const;

  MovementType getMovementType() const;

  int numBodyParts(BodyPart) const;
  int numLost(BodyPart) const;
  int numInjured(BodyPart) const;
  int lostOrInjuredBodyParts() const;
  int numGood(BodyPart) const;
  void injureBodyPart(BodyPart part, bool drop);
  bool isCritical(BodyPart part) const;
  double getMinDamage(BodyPart part) const;

  double getCourage() const;
  void setCourage(double);
  const Gender& getGender() const;

  int getDifficultyPoints() const;
  int getExpLevel() const;
  void exerciseAttr(AttrType, double value = 1);
  void increaseExpLevel(double increase);

  string getDescription() const;
  bool isInnocent() const;

  void addSkill(Skill* skill);
  bool hasSkill(Skill*) const;
  double getSkillValue(const Skill*) const;
  const EnumSet<SkillId>& getDiscreteSkills() const;

  string getPluralTheName(Item* item, int num) const;
  string getPluralAName(Item* item, int num) const;
  CreatureAction move(Position) const;
  CreatureAction move(Vec2) const;
  CreatureAction forceMove(Position) const;
  CreatureAction forceMove(Vec2) const;
  CreatureAction swapPosition(Vec2 direction, bool force = false) const;
  CreatureAction swapPosition(Creature*, bool force = false) const;
  CreatureAction wait() const;
  vector<Item*> getPickUpOptions() const;
  CreatureAction pickUp(const vector<Item*>& item) const;
  CreatureAction drop(const vector<Item*>& item) const;
  void drop(vector<PItem> item);
  struct AttackParams {
    optional<AttackLevel> level;
    enum Mod { WILD, SWIFT};
    optional<Mod> mod;
  };
  CreatureAction attack(Creature*, optional<AttackParams> = none, bool spendTime = true) const;
  CreatureAction bumpInto(Vec2 direction) const;
  CreatureAction applyItem(Item* item) const;
  CreatureAction equip(Item* item) const;
  bool isEquipmentAppropriate(const Item* item) const;
  CreatureAction unequip(Item* item) const;
  bool canEquipIfEmptySlot(const Item* item, string* reason = nullptr) const;
  bool canEquip(const Item* item) const;
  CreatureAction throwItem(Item*, Vec2 direction) const;
  CreatureAction heal(Vec2 direction) const;
  CreatureAction applySquare() const;
  CreatureAction hide() const;
  bool isHidden() const;
  bool knowsHiding(const Creature*) const;
  CreatureAction flyAway() const;
  CreatureAction disappear() const;
  CreatureAction torture(Creature*) const;
  CreatureAction chatTo(Creature*) const;
  CreatureAction stealFrom(Vec2 direction, const vector<Item*>&) const;
  CreatureAction give(Creature* whom, vector<Item*> items);
  CreatureAction fire(Vec2 direction) const;
  CreatureAction construct(Vec2 direction, const SquareType&) const;
  CreatureAction placeTorch(Dir attachmentDir, function<void(Trigger*)> builtCallback) const;
  CreatureAction whip(const Position&) const;
  bool canConstruct(const SquareType&) const;
  CreatureAction eat(Item*) const;
  enum DestroyAction { BASH, EAT, DESTROY };
  CreatureAction destroy(Vec2 direction, DestroyAction) const;
  CreatureAction copulate(Vec2 direction) const;
  bool canCopulateWith(const Creature*) const;
  CreatureAction consume(Creature*) const;
  bool canConsume(const Creature*) const;
  bool isMinionFood() const;
  
  void displace(double time, Vec2);
  void surrender(const Creature* to);
  
  virtual void onChat(Creature*);

  void learnLocation(const Location*);

  Item* getWeapon() const;
  vector<vector<Item*>> stackItems(vector<Item*>) const;

  CreatureAction moveTowards(Position, bool stepOnTile = false);
  CreatureAction moveAway(Position, bool pathfinding = true);
  CreatureAction continueMoving();
  CreatureAction stayIn(const Location*);
  bool isSameSector(Position) const;
  bool canNavigateTo(Position) const;

  bool atTarget() const;
  void die(Creature* attacker = nullptr, bool dropInventory = true, bool dropCorpse = true);
  void die(const string& reason, bool dropInventory = true, bool dropCorpse = true);
  void bleed(double severity);
  void setOnFire(double amount);
  void poisonWithGas(double amount);
  void shineLight();
  void setHeld(const Creature* holding);
  bool isHeld() const;

  void you(MsgType type, const vector<string>& param) const;
  void you(MsgType type, const string& param) const;
  void you(const string& param) const;
  void playerMessage(const PlayerMessage&) const;
  bool isPlayer() const;

  Controller* getController();
  void pushController(PController);
  void setController(PController);
  void popController();
  void setBoulderSpeed(double);
  CreatureSize getSize() const;

  void addCreatureVision(CreatureVision*);
  void removeCreatureVision(CreatureVision*);
  void addSpell(Spell*);
  vector<Spell*> getSpells() const;
  CreatureAction castSpell(Spell*) const;
  CreatureAction castSpell(Spell*, Vec2) const;
  double getSpellDelay(Spell*) const;
  bool isReady(Spell*) const;

  SERIALIZATION_DECL(Creature);

  void addEffect(LastingEffect, double time, bool msg = true);
  void removeEffect(LastingEffect, bool msg = true);
  void addPermanentEffect(LastingEffect, bool msg = true);
  void removePermanentEffect(LastingEffect, bool msg = true);
  bool isAffected(LastingEffect) const;
  bool isAffectedPermanently(LastingEffect) const;
  bool affects(LastingEffect effect) const;
  bool hasFreeMovement() const;
  bool isFireResistant() const;
  bool isDarknessSource() const;

  vector<AttackLevel> getAttackLevels() const;
  bool hasSuicidalAttack() const;

  vector<const Creature*> getUnknownAttacker() const;
  const MinionTaskMap& getMinionTasks() const;
  MinionTaskMap& getMinionTasks();
  int accuracyBonus() const;
  vector<string> getMainAdjectives() const;
  struct AdjectiveInfo {
    string name;
    string help;
  };
  vector<AdjectiveInfo> getGoodAdjectives() const;
  vector<AdjectiveInfo> getWeaponAdjective() const;
  vector<AdjectiveInfo> getBadAdjectives() const;

  vector<string> popPersonalEvents();
  void setInCombat();
  bool wasInCombat(double numLastTurns) const;
  void onKilled(const Creature* victim);
  void onMoved();

  void addSound(const Sound&) const;

  private:

  void onAffected(LastingEffect effect, bool msg);
  void consumeEffects(const EnumMap<LastingEffect, int>&);
  void consumeBodyParts(const EnumMap<BodyPart, int>&);
  void onRemoved(LastingEffect effect, bool msg);
  void onTimedOut(LastingEffect effect, bool msg);
  CreatureAction moveTowards(Position, bool away, bool stepOnTile);
  double getInventoryWeight() const;
  Item* getAmmo() const;
  void updateViewObject();
  AttackType getAttackType() const;
  void spendTime(double time);
  pair<double, double> getStanding(const Creature* c) const;

  HeapAllocated<CreatureAttributes> SERIAL(attributes);
  Position SERIAL(position);
  double SERIAL(localTime) = 1;
  HeapAllocated<Equipment> SERIAL(equipment);
  unique_ptr<LevelShortestPath> SERIAL(shortestPath);
  unordered_set<const Creature*> SERIAL(knownHiding);
  TribeId SERIAL(tribe);
  double SERIAL(health) = 1;
  double SERIAL(morale) = 0;
  optional<double> SERIAL(deathTime);
  bool SERIAL(collapsed) = false;
  bool SERIAL(hidden) = false;
  Creature* SERIAL(lastAttacker) = nullptr;
  optional<string> SERIAL(deathReason);
  int SERIAL(swapPositionCooldown) = 0;
  vector<const Creature*> SERIAL(unknownAttacker);
  vector<const Creature*> SERIAL(privateEnemies);
  const Creature* SERIAL(holding) = nullptr;
  PController SERIAL(controller);
  vector<PController> SERIAL(controllerStack);
  vector<CreatureVision*> SERIAL(creatureVisions);
  vector<const Creature*> SERIAL(kills);
  mutable double SERIAL(difficultyPoints) = 0;
  int SERIAL(points) = 0;
  int SERIAL(numAttacksThisTurn) = 0;
  vector<PMoraleOverride> SERIAL(moraleOverrides);
  void updateVisibleCreatures();
  const vector<Creature*>& getVisibleCreatures();
  vector<const Creature*> SERIAL(visibleEnemies);
  vector<Creature*> SERIAL(visibleCreatures);
  double getTimeRemaining(LastingEffect) const;
  string getRemainingString(LastingEffect) const;
  VisionId SERIAL(vision);
  void updateVision();
  vector<string> SERIAL(personalEvents);
  bool forceMovement = false;
  optional<double> SERIAL(lastCombatTime);

  friend class CreatureListener;
  HeapAllocated<EventGenerator<CreatureListener>> SERIAL(eventGenerator);
};

enum class AttackLevel { LOW, MIDDLE, HIGH };

enum class MsgType {
    FEEL, // better
    BLEEDING_STOPS,
    COLLAPSE,
    FALL,
    FALL_ASLEEP,
    PANIC,
    RAGE,
    DIE_OF,
    ARE, // bleeding
    YOUR, // your head is cut off
    WAKE_UP,
    DIE, //
    TELE_APPEAR,
    TELE_DISAPPEAR,
    ATTACK_SURPRISE,
    CAN_SEE_HIDING,
    SWING_WEAPON,
    THRUST_WEAPON,
    KICK,
    PUNCH,
    BITE,
    HIT,
    TOUCH,
    CRAWL,
    TRIGGER_TRAP,
    DISARM_TRAP,
    DROP_WEAPON,
    GET_HIT_NODAMAGE, // body part
    HIT_THROWN_ITEM,
    HIT_THROWN_ITEM_PLURAL,
    MISS_THROWN_ITEM,
    MISS_THROWN_ITEM_PLURAL,
    ITEM_CRASHES,
    ITEM_CRASHES_PLURAL,
    STAND_UP,
    TURN_INVISIBLE,
    TURN_VISIBLE,
    ENTER_PORTAL,
    HAPPENS_TO,
    BURN,
    DROWN,
    SET_UP_TRAP,
    DECAPITATE,
    TURN,
    BECOME,
    KILLED_BY,
    BREAK_FREE,
    MISS_ATTACK,
    PRAY,
    SACRIFICE,
    COPULATE,
    CONSUME,
    GROW,
};

#endif
