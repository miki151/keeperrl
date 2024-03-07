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

#include "util.h"
#include "enums.h"
#include "unique_entity.h"
#include "renderable.h"
#include "position.h"
#include "owner_pointer.h"
#include "game_time.h"
#include "item_ability.h"

class Level;
class Attack;
class Fire;
class ItemAttributes;
class Effect;
struct CorpseInfo;
struct WeaponInfo;
struct ItemUpgradeInfo;
class ItemPrefix;
class ItemType;
class ContentFactory;
struct AutomatonPart;
struct CreaturePredicate;
class CostInfo;
struct AssembledMinion;

using LastingOrBuff = variant<LastingEffect, BuffId>;

class Item : public Renderable, public UniqueEntity<Item>, public OwnedObject<Item> {
  public:
  Item(const ItemAttributes&, const ContentFactory*);
  virtual ~Item();
  PItem getCopy(const ContentFactory* f) const;

  void apply(Creature*, bool noSound = false);
  bool canApply() const;

  bool isDiscarded();

  string getName(bool plural = false, const Creature* owner = nullptr) const;
  string getTheName(bool plural = false, const Creature* owner = nullptr) const;
  string getAName(bool plural = false, const Creature* owner = nullptr) const;
  string getNameAndModifiers(const ContentFactory*, bool plural = false, const Creature* owner = nullptr) const;
  const optional<string>& getArtifactName() const;
  void setArtifactName(const string&);
  string getShortName(const ContentFactory*, const Creature* owner = nullptr, bool plural = false) const;
  string getPluralName(int count) const;
  string getPluralTheName(int count) const;
  string getPluralAName(int count) const;
  string getPluralTheNameAndVerb(int count, const string& verbSingle, const string& verbPlural) const;

  const optional<Effect>& getEffect() const;
  const optional<AssembledMinion>& getAssembledMinion() const;
  bool effectAppliedWhenThrown() const;
  const optional<CreaturePredicate>& getApplyPredicate() const;
  vector<ItemAbility>& getAbility();
  void upgrade(vector<PItem>, const ContentFactory*);

  ItemClass getClass() const;
  const vector<StorageId>& getStorageIds() const;
  const optional<string>& getEquipmentGroup() const;
  ViewId getEquipedViewId() const;

  CostInfo getCraftingCost() const;
  int getPrice() const;
  void setShopkeeper(const Creature* shopkeeper);
  Creature* getShopkeeper(const Creature* owner) const;
  bool isShopkeeper(const Creature*) const;
  // This function returns true after shopkeeper was killed. TODO: refactor shops.
  bool isOrWasForSale() const;

  optional<CollectiveResourceId> getResourceId() const;
  void setResourceId(optional<CollectiveResourceId>);
  const optional<ItemUpgradeInfo>& getUpgradeInfo() const;
  void setUpgradeInfo(ItemUpgradeInfo);
  vector<ItemUpgradeType> getAppliedUpgradeType() const;
  int getMaxUpgrades() const;
  const optional<string>& getIngredientType() const;

  bool canEquip() const;
  EquipmentSlot getEquipmentSlot() const;
  bool isConflictingEquipment(const Item*) const;
  void addModifier(AttrType, int value);
  int getModifier(AttrType) const;
  const HashMap<AttrType, int>& getModifierValues() const;
  const HashMap<AttrType, pair<int, CreaturePredicate>>& getSpecialModifiers() const;
  void tick(Position, bool carried);
  virtual bool canEverTick(bool carried) const;
  void applyPrefix(const ItemPrefix&, const ContentFactory*);
  void setTimeout(GlobalTime);

  string getApplyMsgThirdPerson(const Creature* owner) const;
  string getApplyMsgFirstPerson(const Creature* owner) const;
  optional<StatId> getProducedStat() const;
  const vector<LastingOrBuff>& getEquipedEffects() const;
  void onEquip(Creature*, bool msg = true, const ContentFactory* = nullptr);
  void onUnequip(Creature*, bool msg = true, const ContentFactory* = nullptr);
  void onOwned(Creature*, bool msg = true, const ContentFactory* factory = nullptr);
  void onDropped(Creature*, bool msg = true, const ContentFactory* factory = nullptr);
  virtual void fireDamage(Position);
  virtual void iceDamage(Position);
  const Fire& getFire() const;

  void onHitSquareMessage(Position, const Attack& attack, int numItems);
  void onHitCreature(Creature* c, const Attack& attack, int numItems);

  TimeInterval getApplyTime() const;
  double getWeight() const;
  vector<string> getDescription(const ContentFactory*) const;

  CreaturePredicate getAutoEquipPredicate() const;
  const string& getEquipWarning() const;
  bool hasOwnedEffect(LastingEffect) const;

  const WeaponInfo& getWeaponInfo() const;
  void getAttackMsg(const Creature*, const string& enemyName) const;

  static ItemPredicate classPredicate(ItemClass);
  static ItemPredicate equipmentSlotPredicate(EquipmentSlot);
  static ItemPredicate classPredicate(vector<ItemClass>);
  static ItemPredicate namePredicate(const string& name);

  static vector<vector<Item*>> stackItems(const ContentFactory*, vector<Item*>,
      function<string(const Item*)> addSuffix = [](const Item*) { return ""; });

  virtual optional<CorpseInfo> getCorpseInfo() const;

  SERIALIZATION_DECL(Item)

  protected:
  virtual void specialTick(Position) {}
  void setName(const string& name);
  bool SERIAL(discarded) = false;
  virtual void applySpecial(Creature*);

  private:
  string getModifiers(const ContentFactory*, bool shorten = false) const;
  string getVisibleName(bool plural) const;
  string getBlindName(bool plural) const;
  HeapAllocated<ItemAttributes> SERIAL(attributes);
  optional<UniqueEntity<Creature>::Id> SERIAL(shopkeeper);
  HeapAllocated<Fire> SERIAL(fire);
  bool SERIAL(canEquipCache);
  ItemClass SERIAL(classCache);
  string getSuffix() const;
  optional<GlobalTime> SERIAL(timeout);
  vector<ItemAbility> SERIAL(abilityInfo);
  void updateAbility(const ContentFactory*);
};
