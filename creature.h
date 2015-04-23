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

#include "creature_attributes.h"
#include "equipment.h"
#include "enums.h"
#include "attack.h"
#include "shortest_path.h"
#include "tribe.h"
#include "skill.h"
#include "controller.h"
#include "unique_entity.h"
#include "event.h"
#include "sectors.h"
#include "vision.h"
#include "square_type.h"
#include "creature_action.h"
#include "renderable.h"
#include "movement_type.h"
#include "player_message.h"
#include "game_info.h"

class Level;
class Tribe;
class EnemyCheck;
class ViewObject;

class Creature : private CreatureAttributes, public Renderable, public UniqueEntity<Creature> {
  public:
  typedef CreatureAttributes CreatureAttributes;
  Creature(Tribe*, const CreatureAttributes&, ControllerFactory);
  Creature(const ViewObject&, Tribe*, const CreatureAttributes&, ControllerFactory);
  virtual ~Creature();

  static string getBodyPartName(BodyPart);

  void makeMove();
  double getTime() const;
  void setTime(double t);
  void setLevel(Level* l);
  vector<const Creature*> getVisibleEnemies() const;
  Level* getLevel();
  const Level* getLevel() const;
  Square* getSquare();
  const Square* getSquare() const;
  Square* getSafeSquare(Vec2 direction);
  const Square* getSafeSquare(Vec2 direction) const;
  vector<Square*> getSquare(Vec2 direction);
  vector<const Square*> getSquare(Vec2 direction) const;
  vector<Square*> getSquares(const vector<Vec2>& direction);
  vector<const Square*> getSquares(const vector<Vec2>& direction) const;
  void setPosition(Vec2 pos);
  Vec2 getPosition() const;
  bool dodgeAttack(Attack);
  bool takeDamage(Attack);
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
  bool canSee(Vec2 pos) const;
  bool isEnemy(const Creature*) const;
  void tick(double realTime);

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
  bool isFriend(const Creature*) const;
  void addEnemyCheck(EnemyCheck*);
  void removeEnemyCheck(EnemyCheck*);
  int getDebt(const Creature* debtor) const;
  vector<Item*> getGold(int num) const;

  void takeItems(vector<PItem> items, const Creature* from);

  void youHit(BodyPart part, AttackType type) const;

  void dropCorpse();
  virtual vector<PItem> getCorpse();
  
  void monsterMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant = "") const;
  void globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant = "") const;

  bool isDead() const;
  bool isBlind() const;
  const Creature* getLastAttacker() const;
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
  MovementType getMovementType() const;

  int numBodyParts(BodyPart) const;
  int numLost(BodyPart) const;
  int numInjured(BodyPart) const;
  int lostOrInjuredBodyParts() const;
  int numGood(BodyPart) const;
  void injureBodyPart(BodyPart part, bool drop);
  bool isCritical(BodyPart part) const;
  double getMinDamage(BodyPart part) const;
  string bodyDescription() const;

  double getCourage() const;
  void setCourage(double);
  Gender getGender() const;

  int getDifficultyPoints() const;
  int getExpLevel() const;
  void exerciseAttr(AttrType, double value = 1);
  void increaseExpLevel(double increase);

  string getDescription() const;
  bool isSpecialMonster() const;
  bool isInnocent() const;

  void addSkill(Skill* skill);
  bool hasSkill(Skill*) const;
  double getSkillValue(const Skill*) const;
  const EnumSet<SkillId>& getDiscreteSkills() const;
  typedef GameInfo::PlayerInfo::SkillInfo SkillInfo;
  vector<SkillInfo> getSkillNames() const;

  string getPluralTheName(Item* item, int num) const;
  string getPluralAName(Item* item, int num) const;
  CreatureAction move(Vec2 direction) const;
  CreatureAction forceMove(Vec2 direction) const;
  CreatureAction swapPosition(Vec2 direction, bool force = false) const;
  CreatureAction wait() const;
  vector<Item*> getPickUpOptions() const;
  CreatureAction pickUp(const vector<Item*>& item, bool spendTime = true) const;
  CreatureAction drop(const vector<Item*>& item) const;
  void drop(vector<PItem> item);
  CreatureAction attack(const Creature*, optional<AttackLevel> = none, bool spendTime = true) const;
  CreatureAction bumpInto(Vec2 direction) const;
  CreatureAction applyItem(Item* item) const;
  CreatureAction equip(Item* item) const;
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
  CreatureAction chatTo(Vec2 direction) const;
  CreatureAction stealFrom(Vec2 direction, const vector<Item*>&) const;
  void give(const Creature* whom, vector<Item*> items);
  CreatureAction fire(Vec2 direction) const;
  CreatureAction construct(Vec2 direction, SquareType) const;
  bool canConstruct(SquareType) const;
  CreatureAction eat(Item*) const;
  enum DestroyAction { BASH, EAT, DESTROY };
  CreatureAction destroy(Vec2 direction, DestroyAction) const;
  CreatureAction copulate(Vec2 direction) const;
  bool canCopulateWith(const Creature*) const;
  CreatureAction consume(Vec2 direction) const;
  bool canConsume(const Creature*) const;
  bool isMinionFood() const;
  
  void surrender(const Creature* to);
  
  virtual void onChat(Creature*);

  void learnLocation(const Location*);

  Item* getWeapon() const;
  vector<vector<Item*>> stackItems(vector<Item*>) const;

  CreatureAction moveTowards(Vec2 pos, bool stepOnTile = false);
  CreatureAction moveAway(Vec2 pos, bool pathfinding = true);
  CreatureAction continueMoving();
  CreatureAction stayIn(const Location*);
  bool isSameSector(Vec2) const;

  bool atTarget() const;
  void die(Creature* attacker = nullptr, bool dropInventory = true, bool dropCorpse = true);
  void bleed(double severity);
  void setOnFire(double amount);
  void poisonWithGas(double amount);
  void shineLight();
  void setHeld(const Creature* holding);
  bool isHeld() const;

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
  bool isFireResistant() const;
  bool isDarknessSource() const;

  vector<AttackLevel> getAttackLevels() const;
  bool hasSuicidalAttack() const;

  vector<const Creature*> getUnknownAttacker() const;
  const MinionTaskMap& getMinionTasks() const;
  int accuracyBonus() const;
  vector<string> getMainAdjectives() const;
  vector<string> getGoodAdjectives() const;
  vector<string> getWeaponAdjective() const;
  vector<string> getBadAdjectives() const;

  vector<string> popPersonalEvents();
  void setInCombat();
  bool wasInCombat(double numLastTurns) const;
  void onKilled(const Creature* victim);

  private:

  double getExpLevelDouble() const;
  double getRawAttr(AttrType) const;
  bool affects(LastingEffect effect) const;
  void onAffected(LastingEffect effect, bool msg);
  void consumeEffects(const EnumMap<LastingEffect, int>&);
  void consumeBodyParts(const EnumMap<BodyPart, int>&);
  void onRemoved(LastingEffect effect, bool msg);
  void onTimedOut(LastingEffect effect, bool msg);
  CreatureAction moveTowards(Vec2 pos, bool away, bool stepOnTile);
  double getInventoryWeight() const;
  Item* getAmmo() const;
  void updateViewObject();
  int getStrengthAttackBonus() const;
  BodyPart getBodyPart(AttackLevel attack) const;
  void injure(BodyPart, bool drop);
  AttackLevel getRandomAttackLevel() const;
  AttackType getAttackType() const;
  void spendTime(double time);
  BodyPart armOrWing() const;
  pair<double, double> getStanding(const Creature* c) const;

  Level* SERIAL(level) = nullptr;
  Vec2 SERIAL(position);
  double SERIAL(time) = 1;
  Equipment SERIAL(equipment);
  optional<ShortestPath> SERIAL(shortestPath);
  unordered_set<const Creature*> SERIAL(knownHiding);
  Tribe* SERIAL(tribe);
  vector<EnemyCheck*> SERIAL(enemyChecks);
  double SERIAL(health) = 1;
  double SERIAL(morale) = 0;
  bool SERIAL(dead) = false;
  double SERIAL(lastTick) = 0;
  bool SERIAL(collapsed) = false;
  EnumMap<BodyPart, int> SERIAL(injuredBodyParts);
  EnumMap<BodyPart, int> SERIAL(lostBodyParts);
  bool SERIAL(hidden) = false;
  Creature* SERIAL(lastAttacker) = nullptr;
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
  EnumMap<LastingEffect, double> SERIAL(lastingEffects);
  vector<PMoraleOverride> SERIAL(moraleOverrides);
  EnumMap<AttrType, double> SERIAL(attrIncrease);
  void updateVisibleCreatures(Rectangle range);
  vector<const Creature*> SERIAL(visibleEnemies);
  double getTimeRemaining(LastingEffect) const;
  string getRemainingString(LastingEffect) const;
  VisionId SERIAL(vision);
  void updateVision();
  vector<string> SERIAL(personalEvents);
  bool forceMovement = false;
  optional<double> SERIAL(lastCombatTime);
};

BOOST_CLASS_VERSION(Creature, 1)

enum class AttackLevel { LOW, MIDDLE, HIGH };

enum class MsgType {
    FEEL, // better
    BLEEDING_STOPS,
    COLLAPSE,
    FALL,
    PANIC,
    RAGE,
    DIE_OF,
    ARE, // bleeding
    YOUR, // your head is cut off
    FALL_ASLEEP, //
    WAKE_UP,
    DIE, //
    FALL_APART,
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
