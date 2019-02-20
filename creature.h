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
class MinionActivityMap;
class CreatureName;
class SpellMap;
class Sound;
class Game;
class CreatureListener;
class CreatureDebt;
class Vision;
struct AdjectiveInfo;
struct MovementInfo;
struct NavigationFlags;

class Creature : public Renderable, public UniqueEntity<Creature>, public OwnedObject<Creature> {
  public:
  Creature(TribeId, CreatureAttributes);
  Creature(const ViewObject&, TribeId, CreatureAttributes);
  virtual ~Creature();

  static vector<vector<Creature*>> stack(const vector<Creature*>&);

  const ViewObject& getViewObjectFor(const Tribe* observer) const;
  void makeMove();
  optional<LocalTime> getLocalTime() const;
  optional<GlobalTime> getGlobalTime() const;
  WLevel getLevel() const;
  Game* getGame() const;
  const vector<Creature*>& getVisibleEnemies() const;
  Creature* getClosestEnemy() const;
  const vector<Creature*>& getVisibleCreatures() const;
  bool shouldAIAttack(const Creature* enemy) const;
  bool shouldAIChase(const Creature* enemy) const;
  vector<Position> getVisibleTiles() const;
  void setGlobalTime(GlobalTime);
  void setPosition(Position);
  Position getPosition() const;
  bool dodgeAttack(const Attack&);
  bool takeDamage(const Attack&);
  void onAttackedBy(Creature*);
  void heal(double amount = 1);
  /** Morale is in the range [-1:1] **/
  double getMorale() const;
  void addMorale(double);
  void take(PItem item);
  void take(vector<PItem> item);
  const Equipment& getEquipment() const;
  Equipment& getEquipment();
  vector<PItem> steal(const vector<Item*> items);
  bool canSeeInPosition(const Creature*) const;
  bool canSeeOutsidePosition(const Creature*) const;
  bool canSee(const Creature*) const;
  bool canSee(Position) const;
  bool canSee(Vec2) const;
  bool isEnemy(const Creature*) const;
  void tick();
  void upgradeViewId(int level);
  ViewId getMaxViewIdUpgrade() const;

  const CreatureName& getName() const;
  CreatureName& getName();
  const char* identify() const;
  int getAttr(AttrType, bool includeWeapon = true) const;
  int getAttrBonus(AttrType, bool includeWeapon) const;

  int getPoints() const;
  const Vision& getVision() const;
  const CreatureDebt& getDebt() const;
  CreatureDebt& getDebt();

  const Tribe* getTribe() const;
  Tribe* getTribe();
  TribeId getTribeId() const;
  void setTribe(TribeId);
  bool isFriend(const Creature*) const;
  vector<Item*> getGold(int num) const;

  void takeItems(vector<PItem> items, Creature* from);
  bool canTakeItems(const vector<Item*>& items) const;

  const CreatureAttributes& getAttributes() const;
  CreatureAttributes& getAttributes();
  const Body& getBody() const;
  Body& getBody();
  bool isDead() const;
  void clearInfoForRetiring();
  optional<string> getDeathReason() const;
  GlobalTime getDeathTime() const;
  const EntitySet<Creature>& getKills() const;

  MovementType getMovementType() const;

  int getDifficultyPoints() const;

  string getPluralTheName(Item* item, int num) const;
  string getPluralAName(Item* item, int num) const;
  CreatureAction move(Position, optional<Position> nextPosIntent = none) const;
  CreatureAction move(Vec2) const;
  CreatureAction forceMove(Position) const;
  CreatureAction forceMove(Vec2) const;
  static CreatureAction wait();
  vector<Item*> getPickUpOptions() const;
  CreatureAction pickUp(const vector<Item*>& item) const;
  bool canCarryMoreWeight(double) const;
  CreatureAction drop(const vector<Item*>& item) const;
  void drop(vector<PItem> item);
  struct AttackParams {
    Item* weapon = nullptr;
    optional<AttackLevel> level;
  };
  CreatureAction attack(Creature*, optional<AttackParams> = none) const;
  int getDefaultWeaponDamage() const;
  CreatureAction execute(Creature*) const;
  CreatureAction applyItem(Item* item) const;
  CreatureAction equip(Item* item) const;
  CreatureAction unequip(Item* item) const;
  bool canEquipIfEmptySlot(const Item* item, string* reason = nullptr) const;
  bool canEquip(const Item* item) const;
  CreatureAction throwItem(Item*, Position target) const;
  optional<int> getThrowDistance(const Item*) const;
  CreatureAction applySquare(Position) const;
  CreatureAction hide() const;
  bool isHidden() const;
  bool knowsHiding(const Creature*) const;
  CreatureAction flyAway() const;
  CreatureAction disappear() const;
  CreatureAction torture(Creature*) const;
  CreatureAction chatTo(Creature*) const;
  CreatureAction pet(Creature*) const;
  CreatureAction stealFrom(Vec2 direction, const vector<Item*>&) const;
  CreatureAction give(Creature* whom, vector<Item*> items) const;
  CreatureAction payFor(const vector<Item*>&) const;
  CreatureAction fire(Position target) const;
  CreatureAction construct(Vec2 direction, FurnitureType) const;
  CreatureAction whip(const Position&) const;
  CreatureAction eat(Item*) const;
  CreatureAction destroy(Vec2 direction, const DestroyAction&) const;
  void destroyImpl(Vec2 direction, const DestroyAction& action);
  CreatureAction copulate(Vec2 direction) const;
  bool canCopulateWith(const Creature*) const;
  CreatureAction consume(Creature*) const;
  bool canConsume(const Creature*) const;
  CreatureAction push(Creature*);

  void displace(Vec2);
  void retire();
  
  void increaseExpLevel(ExperienceType, double increase);

  BestAttack getBestAttack() const;

  Item* getRandomWeapon() const;
  Item* getFirstWeapon() const;
  void dropWeapon();
  void dropUnsupportedEquipment();
  vector<vector<Item*>> stackItems(vector<Item*>) const;
  CreatureAction moveTowards(Position, NavigationFlags);
  CreatureAction moveTowards(Position);
  CreatureAction moveAway(Position, bool pathfinding = true);
  CreatureAction continueMoving();
  CreatureAction stayIn(WLevel, Rectangle);
  bool canNavigateToOrNeighbor(Position) const;
  bool canNavigateTo(Position pos) const;

  bool atTarget() const;

  enum class DropType { NOTHING, ONLY_INVENTORY, EVERYTHING };
  void dieWithAttacker(Creature* attacker, DropType = DropType::EVERYTHING);
  void dieWithLastAttacker(DropType = DropType::EVERYTHING);
  void dieNoReason(DropType = DropType::EVERYTHING);
  void dieWithReason(const string& reason, DropType = DropType::EVERYTHING);

  void affectByFire(double amount);
  void poisonWithGas(double amount);
  void affectBySilver();
  void affectByAcid();
  void setHeld(Creature* holding);
  Creature* getHoldingCreature() const;

  bool isPlayer() const;

  void you(MsgType type, const string& param = "") const;
  void you(const string& param) const;
  void verb(const string& second, const string& third, const string& param = "") const;
  void secondPerson(const PlayerMessage&) const;
  void thirdPerson(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const;
  void thirdPerson(const PlayerMessage& playerCanSee) const;
  void message(const PlayerMessage&) const;
  void privateMessage(const PlayerMessage&) const;
  void addFX(const FXInfo&) const;

  WController getController() const;
  void pushController(PController);
  void setController(PController);
  void popController();

  CreatureAction castSpell(Spell*) const;
  CreatureAction castSpell(Spell*, Position) const;
  TimeInterval getSpellDelay(Spell*) const;
  bool isReady(Spell*) const;

  SERIALIZATION_DECL(Creature)

  bool addEffect(LastingEffect, TimeInterval time, bool msg = true);
  bool removeEffect(LastingEffect, bool msg = true);
  void addPermanentEffect(LastingEffect, int count = 1);
  void removePermanentEffect(LastingEffect, int count = 1);
  bool isAffected(LastingEffect) const;
  optional<TimeInterval> getTimeRemaining(LastingEffect) const;
  bool hasCondition(CreatureCondition) const;

  bool isUnknownAttacker(const Creature*) const;
  vector<AdjectiveInfo> getGoodAdjectives() const;
  vector<AdjectiveInfo> getBadAdjectives() const;

  vector<string> popPersonalEvents();
  void addPersonalEvent(const string&);
  struct CombatIntentInfo {
    Creature* SERIAL(attacker) = nullptr;
    GlobalTime SERIAL(time);
    SERIALIZE_ALL(attacker, time);
  };
  void addCombatIntent(Creature* attacker, bool immediateAttack);
  optional<CombatIntentInfo> getLastCombatIntent() const;
  void onKilledOrCaptured(Creature* victim);

  void addSound(const Sound&) const;
  void updateViewObject();
  void swapPosition(Vec2 direction);
  vector<PItem> generateCorpse(bool instantlyRotten = false) const;
  int getLastMoveCounter() const;

  EnumSet<CreatureStatus>& getStatus();
  const EnumSet<CreatureStatus>& getStatus() const;

  void toggleCaptureOrder();
  bool isCaptureOrdered() const;
  bool canBeCaptured() const;
  void removePrivateEnemy(const Creature*);

  private:

  CreatureAction moveTowards(Position, bool away, NavigationFlags);
  optional<MovementInfo> spendTime(TimeInterval = 1_visible);
  int canCarry(const vector<Item*>&) const;
  TribeSet getFriendlyTribes() const;
  void addMovementInfo(MovementInfo);
  bool canSwapPositionInMovement(Creature* other, optional<Position> nextPos) const;

  HeapAllocated<CreatureAttributes> SERIAL(attributes);
  Position SERIAL(position);
  HeapAllocated<Equipment> SERIAL(equipment);
  heap_optional<LevelShortestPath> SERIAL(shortestPath);
  EntitySet<Creature> SERIAL(knownHiding);
  TribeId SERIAL(tribe);
  double SERIAL(morale) = 0;
  optional<GlobalTime> SERIAL(deathTime);
  bool SERIAL(hidden) = false;
  Creature* lastAttacker = nullptr;
  optional<string> SERIAL(deathReason);
  optional<Position> SERIAL(nextPosIntent);
  EntitySet<Creature> SERIAL(unknownAttackers);
  EntitySet<Creature> SERIAL(privateEnemies);
  optional<Creature::Id> SERIAL(holding);
  vector<PController> SERIAL(controllerStack);
  EntitySet<Creature> SERIAL(kills);
  mutable int SERIAL(difficultyPoints) = 0;
  int SERIAL(points) = 0;
  using MoveId = pair<int, LevelId>;
  MoveId getCurrentMoveId() const;
  mutable optional<pair<MoveId, vector<Creature*>>> visibleEnemies;
  mutable optional<pair<MoveId, vector<Creature*>>> visibleCreatures;
  HeapAllocated<Vision> SERIAL(vision);
  bool forceMovement = false;
  optional<CombatIntentInfo> SERIAL(lastCombatIntent);
  HeapAllocated<CreatureDebt> SERIAL(debt);
  double SERIAL(highestAttackValueEver) = 0;
  int SERIAL(lastMoveCounter) = 0;
  EnumSet<CreatureStatus> SERIAL(statuses);
  bool SERIAL(capture) = 0;
  double SERIAL(captureHealth) = 1;
  bool captureDamage(double damage, Creature* attacker);
  mutable Game* gameCache = nullptr;
  optional<GlobalTime> SERIAL(globalTime);
  void considerMovingFromInaccessibleSquare();
  void updateLastingFX(ViewObject&);
};

struct AdjectiveInfo {
  string name;
  string help;
};
