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
#include "best_attack.h"
#include "msg_type.h"
#include "game_time.h"
#include "creature_status.h"

class Skill;
class Level;
class Tribe;
class ViewObject;
class Attack;
class Controller;
class ControllerFactory;
class PlayerMessage;
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
class Vision;
struct AdjectiveInfo;
struct MovementInfo;

class Creature : public Renderable, public UniqueEntity<Creature>, public OwnedObject<Creature> {
  public:
  Creature(TribeId, CreatureAttributes);
  Creature(const ViewObject&, TribeId, CreatureAttributes);
  virtual ~Creature();

  static vector<vector<WCreature>> stack(const vector<WCreature>&);

  const ViewObject& getViewObjectFor(const Tribe* observer) const;
  void makeMove();
  optional<LocalTime> getLocalTime() const;
  optional<GlobalTime> getGlobalTime() const;
  WLevel getLevel() const;
  WGame getGame() const;
  vector<WCreature> getVisibleEnemies() const;
  vector<WCreature> getVisibleCreatures() const;
  vector<Position> getVisibleTiles() const;
  void setPosition(Position);
  Position getPosition() const;
  bool dodgeAttack(const Attack&);
  bool takeDamage(const Attack&);
  void onAttackedBy(WCreature);
  void heal(double amount = 1);
  /** Morale is in the range [-1:1] **/
  double getMorale() const;
  void addMorale(double);
  void take(PItem item);
  void take(vector<PItem> item);
  const Equipment& getEquipment() const;
  Equipment& getEquipment();
  vector<PItem> steal(const vector<WItem> items);
  bool canSeeInPosition(WConstCreature) const;
  bool canSeeOutsidePosition(WConstCreature) const;
  bool canSee(WConstCreature) const;
  bool canSee(Position) const;
  bool canSee(Vec2) const;
  bool isEnemy(WConstCreature) const;
  void tick();

  const CreatureName& getName() const;
  CreatureName& getName();
  const char* identify() const;
  int getAttr(AttrType) const;

  int getPoints() const;
  const Vision& getVision() const;
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

  const CreatureAttributes& getAttributes() const;
  CreatureAttributes& getAttributes();
  const Body& getBody() const;
  Body& getBody();
  bool isDead() const;
  void clearLastAttacker();
  optional<string> getDeathReason() const;
  GlobalTime getDeathTime() const;
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
    WItem weapon;
    optional<AttackLevel> level;
  };
  CreatureAction attack(WCreature, optional<AttackParams> = none) const;
  CreatureAction execute(WCreature) const;
  CreatureAction bumpInto(Vec2 direction) const;
  CreatureAction applyItem(WItem item) const;
  CreatureAction equip(WItem item) const;
  CreatureAction unequip(WItem item) const;
  bool canEquipIfEmptySlot(WConstItem item, string* reason = nullptr) const;
  bool canEquip(WConstItem item) const;
  CreatureAction throwItem(WItem, Vec2 direction) const;
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
  CreatureAction whip(const Position&) const;
  bool canConstruct(FurnitureType) const;
  CreatureAction eat(WItem) const;
  CreatureAction destroy(Vec2 direction, const DestroyAction&) const;
  void destroyImpl(Vec2 direction, const DestroyAction& action);
  CreatureAction copulate(Vec2 direction) const;
  bool canCopulateWith(WConstCreature) const;
  CreatureAction consume(WCreature) const;
  bool canConsume(WConstCreature) const;
  
  void displace(Vec2);
  void surrender(WCreature to);
  void retire();
  
  void increaseExpLevel(ExperienceType, double increase);

  BestAttack getBestAttack() const;

  WItem getWeapon() const;
  void dropWeapon();
  vector<vector<WItem>> stackItems(vector<WItem>) const;
  struct NavigationFlags {
    NavigationFlags() : stepOnTile(false), destroy(true) {}
    NavigationFlags& requireStepOnTile() {
      stepOnTile = true;
      return *this;
    }
    // This makes the creature stop at the obstacle, and not navigate around it
    NavigationFlags& noDestroying() {
      destroy = false;
      return *this;
    }
    bool stepOnTile;
    bool destroy;
  };
  CreatureAction moveTowards(Position, NavigationFlags = {});
  CreatureAction moveAway(Position, bool pathfinding = true);
  CreatureAction continueMoving();
  CreatureAction stayIn(WLevel, Rectangle);
  bool isSameSector(Position) const;
  bool canNavigateTo(Position) const;

  bool atTarget() const;

  enum class DropType { NOTHING, ONLY_INVENTORY, EVERYTHING };
  void dieWithAttacker(WCreature attacker, DropType = DropType::EVERYTHING);
  void dieWithLastAttacker(DropType = DropType::EVERYTHING);
  void dieNoReason(DropType = DropType::EVERYTHING);
  void dieWithReason(const string& reason, DropType = DropType::EVERYTHING);

  void affectByFire(double amount);
  void poisonWithGas(double amount);
  void affectBySilver();
  void affectByAcid();
  void setHeld(WCreature holding);
  WCreature getHoldingCreature() const;

  bool isPlayer() const;

  void you(MsgType type, const string& param = "") const;
  void you(const string& param) const;
  void verb(const char* second, const char* third, const string& param);
  void secondPerson(const PlayerMessage&) const;
  void thirdPerson(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const;
  void thirdPerson(const PlayerMessage& playerCanSee) const;
  void message(const PlayerMessage&) const;
  void privateMessage(const PlayerMessage&) const;

  WController getController() const;
  void pushController(PController);
  void setController(PController);
  void popController();

  CreatureAction castSpell(Spell*) const;
  CreatureAction castSpell(Spell*, Vec2) const;
  TimeInterval getSpellDelay(Spell*) const;
  bool isReady(Spell*) const;

  SERIALIZATION_DECL(Creature)

  void addEffect(LastingEffect, TimeInterval time, bool msg = true);
  void removeEffect(LastingEffect, bool msg = true);
  void addPermanentEffect(LastingEffect, int count = 1);
  void removePermanentEffect(LastingEffect, int count = 1);
  bool isAffected(LastingEffect) const;
  optional<TimeInterval> getTimeRemaining(LastingEffect) const;
  bool hasCondition(CreatureCondition) const;
  bool isDarknessSource() const;

  bool isUnknownAttacker(WConstCreature) const;
  vector<AdjectiveInfo> getGoodAdjectives() const;
  vector<AdjectiveInfo> getBadAdjectives() const;

  vector<string> popPersonalEvents();
  void addPersonalEvent(const string&);
  struct CombatIntentInfo {
    string SERIAL(attacker);
    GlobalTime SERIAL(time);
    SERIALIZE_ALL(attacker, time)
  };
  void setLastCombatIntent(CombatIntentInfo);
  optional<CombatIntentInfo> getLastCombatIntent() const;
  void onKilled(WCreature victim, optional<ExperienceType> lastDamage);

  void addSound(const Sound&) const;
  void updateViewObject();
  void swapPosition(Vec2 direction);
  vector<PItem> generateCorpse(bool instantlyRotten = false) const;
  int getLastMoveCounter() const;

  EnumSet<CreatureStatus>& getStatus();
  const EnumSet<CreatureStatus>& getStatus() const;

  private:

  CreatureAction moveTowards(Position, bool away, NavigationFlags);
  optional<MovementInfo> spendTime(TimeInterval = 1_visible);
  bool canCarry(const vector<WItem>&) const;
  TribeSet getFriendlyTribes() const;
  void addMovementInfo(MovementInfo);
  bool canSwapPositionInMovement(WCreature other) const;

  HeapAllocated<CreatureAttributes> SERIAL(attributes);
  Position SERIAL(position);
  HeapAllocated<Equipment> SERIAL(equipment);
  unique_ptr<LevelShortestPath> SERIAL(shortestPath);
  EntitySet<Creature> SERIAL(knownHiding);
  TribeId SERIAL(tribe);
  double SERIAL(morale) = 0;
  optional<GlobalTime> SERIAL(deathTime);
  bool SERIAL(hidden) = false;
  WCreature lastAttacker;
  optional<ExperienceType> SERIAL(lastDamageType);
  optional<string> SERIAL(deathReason);
  int SERIAL(swapPositionCooldown) = 0;
  EntitySet<Creature> SERIAL(unknownAttackers);
  EntitySet<Creature> SERIAL(privateEnemies);
  optional<Creature::Id> SERIAL(holding);
  vector<PController> SERIAL(controllerStack);
  EntitySet<Creature> SERIAL(kills);
  mutable int SERIAL(difficultyPoints) = 0;
  int SERIAL(points) = 0;
  void updateVisibleCreatures();
  vector<Position> visibleEnemies;
  vector<Position> visibleCreatures;
  HeapAllocated<Vision> SERIAL(vision);
  bool forceMovement = false;
  optional<CombatIntentInfo> SERIAL(lastCombatIntent);
  HeapAllocated<CreatureDebt> SERIAL(debt);
  double SERIAL(highestAttackValueEver) = 0;
  int SERIAL(lastMoveCounter) = 0;
  EnumSet<CreatureStatus> SERIAL(statuses);
};

struct AdjectiveInfo {
  string name;
  const char* help;
};
