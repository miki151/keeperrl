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
#include "view_object.h"
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

class Level;
class Tribe;

class Creature : public CreatureAttributes, public CreatureView, public UniqueEntity, public EventListener {
  public:
  typedef CreatureAttributes CreatureAttributes;
  Creature(Tribe* tribe, const CreatureAttributes& attr, ControllerFactory);
  Creature(ViewObject, Tribe* tribe, const CreatureAttributes& attr, ControllerFactory);
  virtual ~Creature();

  static Creature* getDefault();
  static Creature* getDefaultMinion();
  static Creature* getDefaultMinionFlyer();
  static void noExperienceLevels();
  static void initialize();

  const ViewObject& getViewObject() const;
  virtual ViewIndex getViewIndex(Vec2 pos) const override;
  void makeMove();
  double getTime() const;
  void setTime(double t);
  void setLevel(Level* l);
  virtual const Level* getLevel() const;
  Level* getLevel();
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
  int getPoints() const;
  vector<string> getMainAdjectives() const;
  vector<string> getAdjectives() const;
  Vision* getVision() const;
  void onKillEvent(const Creature* victim, const Creature* killer) override;

  virtual Tribe* getTribe() const override;
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
  bool hasBrain() const;
  bool isNotLiving() const;
  bool isCorporal() const;
  bool canSwim() const;
  bool canFly() const;
  bool canWalk() const;

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

  void addSkill(Skill* skill);
  bool hasSkill(Skill*) const;
  bool hasSkillToUseWeapon(const Item*) const;
  vector<Skill*> getSkills() const;

  class Action {
    public:
    Action(function<void()>);
    Action(const string& failedReason);
#ifndef RELEASE
    // This stuff is so that you don't forget to perform() an action or check if it failed
    static void checkUsage(bool);
    Action(const Action&);
    ~Action();
#endif
    Action prepend(function<void()>);
    Action append(function<void()>);
    void perform();
    string getFailedReason() const;
    operator bool() const;

    private:
    function<void()> action;
    function<void()> before;
    function<void()> after;
    string failedMessage;
#ifndef RELEASE
    mutable bool wasUsed = false;
#endif
  };

  string getPluralName(Item* item, int num);
  Action move(Vec2 direction);
  Action swapPosition(Vec2 direction, bool force = false);
  Action wait();
  vector<Item*> getPickUpOptions() const;
  Action pickUp(const vector<Item*>& item, bool spendTime = true);
  Action drop(const vector<Item*>& item);
  void drop(vector<PItem> item);
  Action attack(const Creature*, Optional<AttackLevel> = Nothing(), bool spendTime = true);
  Action bumpInto(Vec2 direction);
  Action applyItem(Item* item);
  void startEquipChain();
  void finishEquipChain();
  Action equip(Item* item);
  Action unequip(Item* item);
  bool canEquipIfEmptySlot(const Item* item, string* reason) const;
  bool canEquip(const Item* item) const;
  Action throwItem(Item*, Vec2 direction);
  Action heal(Vec2 direction);
  Action applySquare();
  Action hide();
  bool isHidden() const;
  bool knowsHiding(const Creature*) const;
  Action flyAway();
  Action torture(Creature*);
  Action chatTo(Vec2 direction);
  Action stealFrom(Vec2 direction, const vector<Item*>&);
  void give(const Creature* whom, vector<Item*> items);
  Action fire(Vec2 direction);
  Action construct(Vec2 direction, SquareType);
  bool canConstruct(SquareType) const;
  Action eat(Item*);
  enum DestroyAction { BASH, EAT, DESTROY };
  Action destroy(Vec2 direction, DestroyAction);
  
  virtual void onChat(Creature*);

  void learnLocation(const Location*);

  Item* getWeapon() const;

  Action moveTowards(Vec2 pos, bool stepOnTile = false);
  Action moveAway(Vec2 pos, bool pathfinding = true);
  Action continueMoving();
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
  virtual void refreshGameInfo(View::GameInfo&) const override;
  
  void you(MsgType type, const string& param) const;
  void you(const string& param) const;
  void playerMessage(const string& message) const;
  bool isPlayer() const;
  const MapMemory& getMemory() const override;
  void grantIdentify(int numItems);

  Controller* getController();
  void pushController(PController);
  void popController();
  void setSpeed(double);
  double getSpeed() const;
  CreatureSize getSize() const;

  void addCreatureVision(CreatureVision*);
  void removeCreatureVision(CreatureVision*);

  void addSpell(SpellId);
  const vector<SpellInfo>& getSpells() const;
  Action castSpell(int index);
  static SpellInfo getSpell(SpellId);

  SERIALIZATION_DECL(Creature);

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar & defaultCreature;
    ar & defaultFlyer;
    ar & defaultMinion;
  }

  enum LastingEffect {
    SLEEP, PANIC, RAGE, SLOWED, SPEED, STR_BONUS, DEX_BONUS, HALLU, BLIND, INVISIBLE, POISON, ENTANGLED, STUNNED };

  void addEffect(LastingEffect, double time, bool msg = true);
  void removeEffect(LastingEffect, bool msg = true);
  bool isAffected(LastingEffect) const;

  vector<AttackLevel> getAttackLevels() const;

  private:
  bool affects(LastingEffect effect) const;
  void onAffected(LastingEffect effect, bool msg);
  void onRemoved(Creature::LastingEffect effect, bool msg);
  void onTimedOut(Creature::LastingEffect effect, bool msg);
  string getRemainingString(LastingEffect effect) const;
  static PCreature defaultCreature;
  static PCreature defaultFlyer;
  static PCreature defaultMinion;
  Action moveTowards(Vec2 pos, bool away, bool stepOnTile);
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
  void spendTime(double time);
  BodyPart armOrWing() const;
  pair<double, double> getStanding(const Creature* c) const;

  ViewObject SERIAL(viewObject);
  Level* SERIAL2(level, nullptr);
  Vec2 SERIAL(position);
  double SERIAL2(time, 0);
  Equipment SERIAL(equipment);
  Optional<ShortestPath> SERIAL(shortestPath);
  unordered_set<const Creature*> SERIAL(knownHiding);
  Tribe* SERIAL(tribe);
  vector<EnemyCheck*> SERIAL(enemyChecks);
  double SERIAL2(health, 1);
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
  map<LastingEffect, double> SERIAL(lastingEffects);
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
};

struct SpellInfo {
  SpellId id;
  string name;
  EffectType type;
  double ready;
  int difficulty;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);
};



#endif
