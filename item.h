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

#ifndef _ITEM_H
#define _ITEM_H

#include "util.h"
#include "item_attributes.h"
#include "enums.h"
#include "fire.h"
#include "unique_entity.h"
#include "renderable.h"

class Level;
class Attack;

RICH_ENUM(TrapType,
  BOULDER,
  POISON_GAS,
  ALARM,
  WEB,
  SURPRISE,
  TERROR
);

RICH_ENUM(CollectiveResourceId,
  GOLD,
  WOOD,
  IRON,
  STONE,
  MANA,
  PRISONER_HEAD,
  CORPSE
);

RICH_ENUM(ItemClass,
  WEAPON,
  RANGED_WEAPON,
  AMMO,
  ARMOR,
  SCROLL,
  POTION,
  BOOK,
  AMULET,
  RING,
  TOOL,
  OTHER,
  GOLD,
  FOOD,
  CORPSE
);

class Item : private ItemAttributes, public Renderable, public UniqueEntity<Item> {
  public:
  typedef ItemAttributes ItemAttributes;
  Item(const ItemAttributes&);
  virtual ~Item();

  static string getTrapName(TrapType);

  virtual void apply(Creature*, Level*);

  bool isDiscarded();

  string getName(bool plural = false, bool blind = false) const;
  string getTheName(bool plural = false, bool blind = false) const;
  string getAName(bool plural = false, bool blind = false) const;
  string getNameAndModifiers(bool plural = false, bool blind = false) const;
  string getArtifactName() const;
  string getShortName(bool shortModifiers, bool blind = false) const;

  virtual optional<EffectType> getEffectType() const;
  optional<EffectType> getAttackEffect() const;
  ItemClass getClass() const;
  
  int getPrice() const;
  void setShopkeeper(const Creature* shopkeeper);
  const Creature* getShopkeeper() const;

  optional<TrapType> getTrapType() const;
  optional<CollectiveResourceId> getResourceId() const;

  bool canEquip() const;
  EquipmentSlot getEquipmentSlot() const;
  void addModifier(ModifierType, int value);
  int getModifier(ModifierType) const;
  int getAttr(AttrType) const;

  void tick(double time, Level*, Vec2 position);
  
  string getApplyMsgThirdPerson(bool blind) const;
  string getApplyMsgFirstPerson(bool blind) const;
  string getNoSeeApplyMsg() const;

  void onEquip(Creature*);
  void onUnequip(Creature*);
  virtual void onEquipSpecial(Creature*) {}
  virtual void onUnequipSpecial(Creature*) {}
  virtual void setOnFire(double amount, const Level* level, Vec2 position);
  double getFireSize() const;

  void onHitSquareMessage(Vec2 position, Square*, bool plural);
  void onHitCreature(Creature* c, const Attack& attack, bool plural);

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

  struct CorpseInfo {
    UniqueEntity<Creature>::Id victim;
    bool canBeRevived;
    bool hasHead;
    bool isSkeleton;

    template <class Archive> 
    void serialize(Archive& ar, const unsigned int version);
  };

  virtual optional<CorpseInfo> getCorpseInfo() const { return none; }

  SERIALIZATION_DECL(Item);

  protected:
  virtual void specialTick(double time, Level*, Vec2 position) {}
  void setName(const string& name);
  bool SERIAL2(discarded, false);

  private:
  string getModifiers(bool shorten = false) const;
  string getVisibleName(bool plural) const;
  string getBlindName(bool plural) const;
  const Creature* SERIAL2(shopkeeper, nullptr);
  Fire SERIAL(fire);
};

BOOST_CLASS_VERSION(Item, 1)

#endif
