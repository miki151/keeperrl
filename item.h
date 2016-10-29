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

class Level;
class Attack;
class Fire;
class ItemAttributes;
class EffectType;
struct CorpseInfo;

RICH_ENUM(TrapType,
  BOULDER,
  POISON_GAS,
  ALARM,
  WEB,
  SURPRISE,
  TERROR
);

class Item : public Renderable, public UniqueEntity<Item> {
  public:
  Item(const ItemAttributes&);
  virtual ~Item();

  static string getTrapName(TrapType);

  void apply(Creature*, bool noSound = false);

  bool isDiscarded();

  string getName(bool plural = false, const Creature* owner = nullptr) const;
  string getTheName(bool plural = false, const Creature* owner = nullptr) const;
  string getAName(bool plural = false, const Creature* owner = nullptr) const;
  string getNameAndModifiers(bool plural = false, const Creature* owner = nullptr) const;
  string getArtifactName() const;
  string getShortName(const Creature* owner = nullptr, bool noSuffix = false) const;
  string getPluralName(int count) const;
  string getPluralTheName(int count) const;
  string getPluralTheNameAndVerb(int count, const string& verbSingle, const string& verbPlural) const;

  const optional<EffectType>& getEffectType() const;
  optional<EffectType> getAttackEffect() const;
  ItemClass getClass() const;
  
  int getPrice() const;
  void setShopkeeper(const Creature* shopkeeper);
  const Creature* getShopkeeper(const Creature* owner) const;
  bool isShopkeeper(const Creature*) const;
  // This function returns true after shopkeeper was killed. TODO: refactor shops.
  bool isOrWasForSale() const;

  optional<TrapType> getTrapType() const;
  optional<CollectiveResourceId> getResourceId() const;

  bool canEquip() const;
  EquipmentSlot getEquipmentSlot() const;
  void addModifier(ModifierType, int value);
  int getModifier(ModifierType) const;
  int getAttr(AttrType) const;

  void tick(Position);
  
  string getApplyMsgThirdPerson(const Creature* owner) const;
  string getApplyMsgFirstPerson(const Creature* owner) const;
  string getNoSeeApplyMsg() const;

  void onEquip(Creature*);
  void onUnequip(Creature*);
  virtual void onEquipSpecial(Creature*) {}
  virtual void onUnequipSpecial(Creature*) {}
  virtual void fireDamage(double amount, Position);
  double getFireSize() const;

  void onHitSquareMessage(Position, int numItems);
  void onHitCreature(Creature* c, const Attack& attack, int numItems);

  double getApplyTime() const;
  double getWeight() const;
  string getDescription() const;

  AttackType getAttackType() const;
  bool isWieldedTwoHanded() const;
  int getMinStrength() const;

  static ItemPredicate effectPredicate(EffectType);
  static ItemPredicate classPredicate(ItemClass);
  static ItemPredicate classPredicate(vector<ItemClass>);
  static ItemPredicate namePredicate(const string& name);

  static vector<pair<string, vector<Item*>>> stackItems(vector<Item*>,
      function<string(const Item*)> addSuffix = [](const Item*) { return ""; });

  virtual optional<CorpseInfo> getCorpseInfo() const;

  SERIALIZATION_DECL(Item);

  protected:
  virtual void specialTick(Position) {}
  void setName(const string& name);
  bool SERIAL(discarded) = false;
  virtual void applySpecial(Creature*);

  private:
  string getModifiers(bool shorten = false) const;
  string getVisibleName(bool plural) const;
  string getBlindName(bool plural) const;
  HeapAllocated<ItemAttributes> SERIAL(attributes);
  optional<UniqueEntity<Creature>::Id> SERIAL(shopkeeper);
  HeapAllocated<Fire> SERIAL(fire);
  bool SERIAL(canEquipCache);
  ItemClass SERIAL(classCache);
};

BOOST_CLASS_VERSION(Item, 1)
