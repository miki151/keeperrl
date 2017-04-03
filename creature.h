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

#pragma once

#include "enums.h"
#include "unique_entity.h"
#include "creature_action.h"
#include "renderable.h"
#include "movement_type.h"
#include "position.h"
#include "event_generator.h"
#include "entity_set.h"
#include "destroy_action.h"

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
class ShortestPath;
class LevelShortestPath;
class Equipment;
class Spell;
class CreatureAttributes;
class Body;
class MinionTaskMap;
class CreatureName;
class Gender;
class SpellMap;
class Sound;
class Game;
class CreatureListener;
class CreatureDebt;
struct AdjectiveInfo;
struct MovementInfo;

class Creature : public Renderable, public UniqueEntity<Creature>, public OwnedObject<Creature> {
  public:
  Creature(TribeId, const CreatureAttributes&);
  Creature(const ViewObject&, TribeId, const CreatureAttributes&);
  virtual ~Creature();

  static vector<vector<WCreature>> stack(const vector<WCreature>&);

  const ViewObject& getViewObjectFor(const Tribe* observer) const;
  void makeMove();
  double getLocalTime() const;
  double getGlobalTime() const;
  WLevel getLevel() const;
  WGame getGame() const;
  vector<WCreature> getVisibleEnemies() const;
  vector<WCreature> getVisibleCreatures() const;
  vector<Position> getVisibleTiles() const;
  void setPosition(Position);
  Position getPosition() const;
  bool dodgeAttack(const Attack&);
  bool takeDamage(const Attack&);
  void heal(double amount = 1, bool replaceLimbs = false);
  /** Morale is in the range [-1:1] **/
  double getMorale() const;
  void addMorale(double);
  DEF_UNIQUE_PTR(MoraleOverride);
  class MoraleOverride {
    public:
    virtual optional<double> getMorale(WConstCreature) = 0;
    virtual ~MoraleOverride() {}
    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };
  void setMoraleOverride(PMoraleOverride);

  void take(PItem item);
  void take(vector<PItem> item);
  const Equipment& getEquipment() const;
  Equipment& getEquipment();
  vector<PItem> steal(const vector<WItem> items);
  bool canSee(WConstCreature) const;
  bool canSee(Position) const;
  bool canSee(Vec2) const;
  bool isEnemy(WConstCreature) const;
  void tick();

  const CreatureName& getName() const;
  CreatureName& getName();
  int getModifier(ModifierType) const;
  int getAttr(AttrType) const;
  static string getAttrName(AttrType);
  static string getModifierName(ModifierType);

  int getPoints() const;
  VisionId getVision() const;
  const CreatureDebt& getDebt() const;
  CreatureDebt& getDebt();

  const Tribe* getTribe() const;
  Tribe* getTribe();
  TribeId getTribeId() const;
  void setTribe(TribeId);
  bool isFriend(WConstCreature) const;
  vector<WItem> getGold(int num) const;

  void takeItems(vector<PItem> items, WCreature from);
  bool canTakeItems(const vector<WItem>& items) const;

  void youHit(BodyPart part, AttackType type) const;

  void monsterMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const;
  void monsterMessage(const PlayerMessage& playerCanSee) const;
  void globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const;
  void globalMessage(const PlayerMessage& playerCanSee) const;

  const CreatureAttributes& getAttributes() const;
  CreatureAttributes& getAttributes();
  const Body& getBody() const;
  Body& getBody();
  bool isDead() const;
  bool isBlind() const;
  void clearLastAttacker();
  optional<string> getDeathReason() const;
  double getDeathTime() const;
  const EntitySet<Creature>& getKills() const;

  MovementType getMovementType() const;

  int getDifficultyPoints() const;

  void addSkill(Skill* skill);

  string getPluralTheName(WItem item, int num) const;
  string getPluralAName(WItem item, int num) const;
  CreatureAction move(Position) const;
  CreatureAction move(Vec2) const;
  CreatureAction forceMove(Position) const;
  CreatureAction forceMove(Vec2) const;
  CreatureAction wait() const;
  vector<WItem> getPickUpOptions() const;
  CreatureAction pickUp(const vector<WItem>& item) const;
  CreatureAction drop(const vector<WItem>& item) const;
  void drop(vector<PItem> item);
  struct AttackParams {
    optional<AttackLevel> level;
    enum Mod { WILD, SWIFT};
    optional<Mod> mod;
  };
  CreatureAction attack(WCreature, optional<AttackParams> = none, bool spendTime = true) const;
  CreatureAction bumpInto(Vec2 direction) const;
  CreatureAction applyItem(WItem item) const;
  CreatureAction equip(WItem item) const;
  bool isEquipmentAppropriate(const WItem item) const;
  CreatureAction unequip(WItem item) const;
  bool canEquipIfEmptySlot(const WItem item, string* reason = nullptr) const;
  bool canEquip(const WItem item) const;
  CreatureAction throwItem(WItem, Vec2 direction) const;
  CreatureAction heal(Vec2 direction) const;
  CreatureAction applySquare(Position) const;
  CreatureAction hide() const;
  bool isHidden() const;
  bool knowsHiding(WConstCreature) const;
  CreatureAction flyAway() const;
  CreatureAction disappear() const;
  CreatureAction torture(WCreature) const;
  CreatureAction chatTo(WCreature) const;
  CreatureAction stealFrom(Vec2 direction, const vector<WItem>&) const;
  CreatureAction give(WCreature whom, vector<WItem> items) const;
  CreatureAction payFor(const vector<WItem>&) const;
  CreatureAction fire(Vec2 direction) const;
  CreatureAction construct(Vec2 direction, FurnitureType) const;
  CreatureAction placeTorch(Dir attachmentDir, function<void(Trigger*)> builtCallback) const;
  CreatureAction whip(const Position&) const;
  bool canConstruct(const SquareType&) const;
  bool canConstruct(FurnitureType) const;
  CreatureAction eat(WItem) const;
  CreatureAction destroy(Vec2 direction, const DestroyAction&) const;
  void destroyImpl(Vec2 direction, const DestroyAction& action);
  CreatureAction copulate(Vec2 direction) const;
  bool canCopulateWith(WConstCreature) const;
  CreatureAction consume(WCreature) const;
  bool canConsume(WConstCreature) const;
  
  void displace(double time, Vec2);
  void surrender(WCreature to);
  void retire();
  
  void increaseExpLevel(ExperienceType, double increase);

  WItem getWeapon() const;
  void dropWeapon();
  vector<vector<WItem>> stackItems(vector<WItem>) const;

  CreatureAction moveTowards(Position, bool stepOnTile = false);
  CreatureAction moveAway(Position, bool pathfinding = true);
  CreatureAction continueMoving();
  CreatureAction stayIn(WLevel, Rectangle);
  bool isSameSector(Position) const;
  bool canNavigateTo(Position) const;

  bool atTarget() const;
  void die(WCreature attacker, bool dropInventory = true, bool dropCorpse = true);
  void die(bool dropInventory = true, bool dropCorpse = true);
  void die(const string& reason, bool dropInventory = true, bool dropCorpse = true);
  void fireDamage(double amount);
  void poisonWithGas(double amount);
  void affectBySilver();
  void affectByAcid();
  void setHeld(WCreature holding);
  WCreature getHoldingCreature() const;

  void you(MsgType type, const vector<string>& param) const;
  void you(MsgType type, const string& param) const;
  void you(const string& param) const;
  void playerMessage(const PlayerMessage&) const;
  bool isPlayer() const;

  WController getController() const;
  void pushController(PController);
  void setController(PController);
  void popController();

  void addCreatureVision(WCreatureVision);
  void removeCreatureVision(WCreatureVision);
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
  optional<double> getTimeRemaining(LastingEffect) const;
  bool hasCondition(CreatureCondition) const;
  bool isDarknessSource() const;

  bool isUnknownAttacker(WConstCreature) const;
  int accuracyBonus() const;
  vector<string> getMainAdjectives() const;
  vector<AdjectiveInfo> getGoodAdjectives() const;
  vector<AdjectiveInfo> getWeaponAdjective() const;
  vector<AdjectiveInfo> getBadAdjectives() const;

  vector<string> popPersonalEvents();
  void addPersonalEvent(const string&);
  void setInCombat();
  bool wasInCombat(double numLastTurns) const;
  void onKilled(WCreature victim);

  void addSound(const Sound&) const;
  void updateViewObject();
  void swapPosition(Vec2 direction);

  private:

  CreatureAction moveTowards(Position, bool away, bool stepOnTile);
  WItem getAmmo() const;
  void spendTime(double time);
  bool canCarry(const vector<WItem>&) const;
  TribeSet getFriendlyTribes() const;
  void addMovementInfo(const MovementInfo&);
  bool canSwapPositionInMovement(WCreature other) const;

  HeapAllocated<CreatureAttributes> SERIAL(attributes);
  Position SERIAL(position);
  HeapAllocated<Equipment> SERIAL(equipment);
  unique_ptr<LevelShortestPath> SERIAL(shortestPath);
  EntitySet<Creature> SERIAL(knownHiding);
  TribeId SERIAL(tribe);
  double SERIAL(morale) = 0;
  optional<double> SERIAL(deathTime);
  bool SERIAL(hidden) = false;
  WCreature lastAttacker = nullptr;
  optional<string> SERIAL(deathReason);
  int SERIAL(swapPositionCooldown) = 0;
  EntitySet<Creature> SERIAL(unknownAttackers);
  EntitySet<Creature> SERIAL(privateEnemies);
  optional<Creature::Id> SERIAL(holding);
  vector<PController> SERIAL(controllerStack);
  vector<WCreatureVision> SERIAL(creatureVisions);
  EntitySet<Creature> SERIAL(kills);
  mutable double SERIAL(difficultyPoints) = 0;
  int SERIAL(points) = 0;
  int SERIAL(numAttacksThisTurn) = 0;
  PMoraleOverride SERIAL(moraleOverride);
  void updateVisibleCreatures();
  vector<Position> visibleEnemies;
  vector<Position> visibleCreatures;
  VisionId SERIAL(vision);
  void updateVision();
  bool forceMovement = false;
  optional<double> SERIAL(lastCombatTime);
  HeapAllocated<CreatureDebt> SERIAL(debt);
};

BOOST_CLASS_VERSION(Creature, 2);

struct AdjectiveInfo {
  string name;
  string help;
};

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

