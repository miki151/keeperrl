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
#include "map_memory.h"
#include "creature_view.h"
#include "controller.h"
#include "unique_entity.h"
#include "event.h"
#include "sectors.h"
#include "vision.h"
#include "square_type.h"
#include "creature_action.h"
#include "spell_info.h"
#include "renderable.h"

class Level;
class Tribe;
class EnemyCheck;
class ViewObject;

class Creature : private CreatureAttributes, public Renderable, public CreatureView, public UniqueEntity,
    public EventListener {
  public:
  typedef CreatureAttributes CreatureAttributes;
  Creature(Tribe* tribe, const CreatureAttributes& attr, ControllerFactory);
  Creature(const ViewObject&, Tribe* tribe, const CreatureAttributes& attr, ControllerFactory);
  virtual ~Creature();

  static Creature* getDefault();
  static Creature* getDefaultMinion();
  static Creature* getDefaultMinionFlyer();
  static void noExperienceLevels();
  static void initialize();

  virtual ViewIndex getViewIndex(Vec2 pos) const override;
  void makeMove();
  double getTime() const;
  void setTime(double t);
  void setLevel(Level* l);
  virtual const Level* getViewLevel() const override;
  Level* getLevel();
  const Level* getLevel() const;
  const Square* getConstSquare() const;
  const Square* getConstSquare(Vec2 direction) const;
  Square* getSquare();
  Square* getSquare(Vec2 direction);
  void setPosition(Vec2 pos);
  virtual Vec2 getPosition() const override;
  bool dodgeAttack(const Attack&);
  bool takeDamage(const Attack&);
  void heal(double amount = 1, bool replaceLimbs = false);
  double getHealth() const;
  double getMorale() const;
  void addMorale(double);
  class MoraleOverride {
    public:
    virtual Optional<double> getMorale() = 0;
    virtual ~MoraleOverride() {}
    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };
  DEF_UNIQUE_PTR(MoraleOverride);
  void addMoraleOverride(PMoraleOverride);

  double getWeight() const;
  bool canSleep() const;
  void take(PItem item);
  void take(vector<PItem> item);
  const Equipment& getEquipment() const;
  Equipment& getEquipment();
  vector<PItem> steal(const vector<Item*> items);
  virtual bool canSee(const Creature*) const override;
  virtual bool canSee(Vec2 pos) const override;
  virtual bool isEnemy(const Creature*) const override;
  void tick(double realTime);

  string getTheName() const;
  string getAName() const;
  string getName() const;
  string getSpeciesName() const;
  string getNameAndTitle() const;
  Optional<string> getFirstName() const;
  int getAttr(AttrType) const;
  static string getAttrName(AttrType attr);

  int getPoints() const;
  vector<string> getMainAdjectives() const;
  vector<string> getAdjectives() const;
  Vision* getVision() const;
  void onKillEvent(const Creature* victim, const Creature* killer) override;

  virtual const Tribe* getTribe() const override;
  Tribe* getTribe();
  bool isFriend(const Creature*) const;
  void addEnemyCheck(EnemyCheck*);
  void removeEnemyCheck(EnemyCheck*);
  int getDebt(const Creature* debtor) const;
  vector<Item*> getGold(int num) const;

  void takeItems(vector<PItem> items, const Creature* from);

  void youHit(BodyPart part, AttackType type) const;

  virtual void dropCorpse();
  
  void monsterMessage(const string& playerCanSee, const string& cant = "") const;
  void globalMessage(const string& playerCanSee, const string& cant = "") const;

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
  void makeUndead();
  bool hasBrain() const;
  bool isNotLiving() const;
  bool isCorporal() const;
  bool isWorshipped() const;
  bool canSwim() const;
  bool canFly() const;
  bool canWalk() const;
  bool canBeMinion() const;

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
  Gender getGender() const;

  void increaseExpLevel(double);
  int getExpLevel() const;
  int getDifficultyPoints() const;

  string getDescription() const;
  bool isSpecialMonster() const;
  bool isInnocent() const;

  void addSkill(Skill* skill);
  bool hasSkill(Skill*) const;
  vector<Skill*> getSkills() const;

  string getPluralName(Item* item, int num);
  CreatureAction move(Vec2 direction);
  CreatureAction swapPosition(Vec2 direction, bool force = false);
  CreatureAction wait();
  vector<Item*> getPickUpOptions() const;
  CreatureAction pickUp(const vector<Item*>& item, bool spendTime = true);
  CreatureAction drop(const vector<Item*>& item);
  void drop(vector<PItem> item);
  CreatureAction attack(const Creature*, Optional<AttackLevel> = Nothing(), bool spendTime = true);
  CreatureAction bumpInto(Vec2 direction);
  CreatureAction applyItem(Item* item);
  void startEquipChain();
  void finishEquipChain();
  CreatureAction equip(Item* item);
  CreatureAction unequip(Item* item);
  bool canEquipIfEmptySlot(const Item* item, string* reason = nullptr) const;
  bool canEquip(const Item* item) const;
  CreatureAction throwItem(Item*, Vec2 direction);
  CreatureAction heal(Vec2 direction);
  CreatureAction applySquare();
  CreatureAction hide();
  bool isHidden() const;
  bool knowsHiding(const Creature*) const;
  CreatureAction flyAway();
  CreatureAction disappear();
  CreatureAction torture(Creature*);
  CreatureAction chatTo(Vec2 direction);
  CreatureAction stealFrom(Vec2 direction, const vector<Item*>&);
  void give(const Creature* whom, vector<Item*> items);
  CreatureAction fire(Vec2 direction);
  CreatureAction construct(Vec2 direction, SquareType);
  bool canConstruct(SquareType) const;
  CreatureAction eat(Item*);
  enum DestroyAction { BASH, EAT, DESTROY };
  CreatureAction destroy(Vec2 direction, DestroyAction);
  
  virtual void onChat(Creature*);

  void learnLocation(const Location*);

  Item* getWeapon() const;

  CreatureAction moveTowards(Vec2 pos, bool stepOnTile = false);
  CreatureAction moveAway(Vec2 pos, bool pathfinding = true);
  CreatureAction continueMoving();
  CreatureAction stayIn(const Location*);
  void addSectors(Sectors*);

  bool atTarget() const;
  void die(const Creature* attacker = nullptr, bool dropInventory = true, bool dropCorpse = true);
  void bleed(double severity);
  void setOnFire(double amount);
  void poisonWithGas(double amount);
  void shineLight();
  void setHeld(const Creature* holding);
  bool isHeld() const;

  virtual vector<const Creature*> getUnknownAttacker() const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  
  void you(MsgType type, const string& param) const;
  void you(const string& param) const;
  void playerMessage(const string& message) const;
  bool isPlayer() const;
  const MapMemory& getMemory() const override;
  void grantIdentify(int numItems);

  Controller* getController();
  void pushController(PController);
  void setController(PController);
  void popController();
  void setSpeed(double);
  double getSpeed() const;
  CreatureSize getSize() const;

  void addCreatureVision(CreatureVision*);
  void removeCreatureVision(CreatureVision*);

  void addSpell(SpellId);
  const vector<SpellInfo>& getSpells() const;
  CreatureAction castSpell(int index);
  static SpellInfo getSpell(SpellId);

  SERIALIZATION_DECL(Creature);

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar & defaultCreature;
    ar & defaultFlyer;
    ar & defaultMinion;
  }

  void addEffect(LastingEffect, double time, bool msg = true);
  void removeEffect(LastingEffect, bool msg = true);
  void addPermanentEffect(LastingEffect, bool msg = true);
  void removePermanentEffect(LastingEffect, bool msg = true);
  bool isAffected(LastingEffect) const;
  bool isAffectedPermanently(LastingEffect) const;

  vector<AttackLevel> getAttackLevels() const;

  private:
  bool affects(LastingEffect effect) const;
  void onAffected(LastingEffect effect, bool msg);
  void onRemoved(LastingEffect effect, bool msg);
  void onTimedOut(LastingEffect effect, bool msg);
  string getRemainingString(LastingEffect effect) const;
  static PCreature defaultCreature;
  static PCreature defaultFlyer;
  static PCreature defaultMinion;
  CreatureAction moveTowards(Vec2 pos, bool away, bool stepOnTile);
  double getInventoryWeight() const;
  Item* getAmmo() const;
  void updateViewObject();
  int getStrengthAttackBonus() const;
  int getAttrVal(AttrType type) const;
  int getToHit() const;
  BodyPart getBodyPart(AttackLevel attack) const;
  bool isFireResistant() const;
  void injure(BodyPart, bool drop);
  AttackLevel getRandomAttackLevel() const;
  AttackType getAttackType() const;
  int toHitBonus() const;
  void spendTime(double time);
  BodyPart armOrWing() const;
  pair<double, double> getStanding(const Creature* c) const;

  Level* SERIAL2(level, nullptr);
  Vec2 SERIAL(position);
  double SERIAL2(time, 1);
  Equipment SERIAL(equipment);
  Optional<ShortestPath> SERIAL(shortestPath);
  unordered_set<const Creature*> SERIAL(knownHiding);
  Tribe* SERIAL(tribe);
  vector<EnemyCheck*> SERIAL(enemyChecks);
  double SERIAL2(health, 1);
  double SERIAL2(morale, 0);
  bool SERIAL2(dead, false);
  double SERIAL2(lastTick, 0);
  bool SERIAL2(collapsed, false);
  EnumMap<BodyPart, int> SERIAL(injuredBodyParts);
  EnumMap<BodyPart, int> SERIAL(lostBodyParts);
  bool SERIAL2(hidden, false);
  bool inEquipChain = false;
  int numEquipActions = 0;
  const Creature* SERIAL2(lastAttacker, nullptr);
  int SERIAL2(swapPositionCooldown, 0);
  double SERIAL2(expLevel, 1);
  vector<const Creature*> SERIAL(unknownAttacker);
  vector<const Creature*> SERIAL(privateEnemies);
  const Creature* SERIAL2(holding, nullptr);
  PController SERIAL(controller);
  vector<PController> SERIAL(controllerStack);
  vector<CreatureVision*> SERIAL(creatureVisions);
  mutable vector<const Creature*> SERIAL(kills);
  mutable double SERIAL2(difficultyPoints, 0);
  int SERIAL2(points, 0);
  Sectors* SERIAL2(sectors, nullptr);
  int SERIAL2(numAttacksThisTurn, 0);
  EnumMap<LastingEffect, double> SERIAL(lastingEffects);
  vector<PMoraleOverride> SERIAL(moraleOverrides);
};

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
    CRAWL,
    TRIGGER_TRAP,
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
    KILLED_BY,
    BREAK_FREE,
    MISS_ATTACK,
    PRAY,
    SACRIFICE};

#endif
