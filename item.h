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

class Level;
class Attack;
class Fire;
class ItemAttributes;
class Effect;
struct CorpseInfo;
class RangedWeapon;
class WeaponInfo;

class Item : public Renderable, public UniqueEntity<Item>, public OwnedObject<Item> {
  public:
  Item(const ItemAttributes&);
  virtual ~Item();
  PItem getCopy() const;

  void apply(WCreature, bool noSound = false);

  bool isDiscarded();

  string getName(bool plural = false, WConstCreature owner = nullptr) const;
  string getTheName(bool plural = false, WConstCreature owner = nullptr) const;
  string getAName(bool plural = false, WConstCreature owner = nullptr) const;
  string getNameAndModifiers(bool plural = false, WConstCreature owner = nullptr) const;
  const optional<string>& getArtifactName() const;
  void setArtifactName(const string&);
  string getShortName(WConstCreature owner = nullptr, bool plural = false) const;
  string getPluralName(int count) const;
  string getPluralTheName(int count) const;
  string getPluralTheNameAndVerb(int count, const string& verbSingle, const string& verbPlural) const;

  const optional<Effect>& getEffect() const;
  ItemClass getClass() const;
  
  int getPrice() const;
  void setShopkeeper(WConstCreature shopkeeper);
  WCreature getShopkeeper(WConstCreature owner) const;
  bool isShopkeeper(WConstCreature) const;
  // This function returns true after shopkeeper was killed. TODO: refactor shops.
  bool isOrWasForSale() const;

  optional<TrapType> getTrapType() const;
  optional<CollectiveResourceId> getResourceId() const;

  bool canEquip() const;
  EquipmentSlot getEquipmentSlot() const;
  void addModifier(AttrType, int value);
  int getModifier(AttrType) const;
  const optional<RangedWeapon>& getRangedWeapon() const;
  void tick(Position);
  bool applyRandomPrefix();
  void setTimeout(GlobalTime);
  
  string getApplyMsgThirdPerson(WConstCreature owner) const;
  string getApplyMsgFirstPerson(WConstCreature owner) const;
  string getNoSeeApplyMsg() const;

  void onEquip(WCreature);
  void onUnequip(WCreature);
  void onOwned(WCreature);
  void onDropped(WCreature);
  virtual void fireDamage(double amount, Position);
  double getFireSize() const;

  void onHitSquareMessage(Position, int numItems);
  void onHitCreature(WCreature c, const Attack& attack, int numItems);

  TimeInterval getApplyTime() const;
  double getWeight() const;
  string getDescription() const;
  optional<LastingEffect> getOwnedEffect() const;

  const WeaponInfo& getWeaponInfo() const;
  void getAttackMsg(const Creature*, const string& enemyName) const;

  static ItemPredicate effectPredicate(Effect);
  static ItemPredicate classPredicate(ItemClass);
  static ItemPredicate equipmentSlotPredicate(EquipmentSlot);
  static ItemPredicate classPredicate(vector<ItemClass>);
  static ItemPredicate namePredicate(const string& name);
  static ItemPredicate isRangedWeaponPredicate();

  static vector<vector<WItem>> stackItems(vector<WItem>,
      function<string(WConstItem)> addSuffix = [](WConstItem) { return ""; });

  virtual optional<CorpseInfo> getCorpseInfo() const;

  SERIALIZATION_DECL(Item);

  protected:
  virtual void specialTick(Position) {}
  void setName(const string& name);
  bool SERIAL(discarded) = false;
  virtual void applySpecial(WCreature);

  private:
  string getModifiers(bool shorten = false) const;
  string getVisibleName(bool plural) const;
  string getBlindName(bool plural) const;
  HeapAllocated<ItemAttributes> SERIAL(attributes);
  optional<UniqueEntity<Creature>::Id> SERIAL(shopkeeper);
  HeapAllocated<Fire> SERIAL(fire);
  bool SERIAL(canEquipCache);
  ItemClass SERIAL(classCache);
  string getSuffix() const;
  optional<GlobalTime> SERIAL(timeout);
};
