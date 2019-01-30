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

#include "stdafx.h"

#include "item_factory.h"
#include "creature_factory.h"
#include "lasting_effect.h"
#include "effect.h"
#include "item_type.h"

SERIALIZE_DEF(ItemFactory, items, weights, count, uniqueCounts)
SERIALIZATION_CONSTRUCTOR_IMPL(ItemFactory)

struct ItemFactory::ItemInfo {
  ItemInfo(ItemType _id, double _weight) : id(_id), weight(_weight), count(1, 2) {}
  ItemInfo(ItemType _id, double _weight, Range c)
    : id(_id), weight(_weight), count(c) {}

  ItemType id;
  double weight;
  Range count;
};

ItemFactory::ItemFactory(const vector<ItemInfo>& items) {
  for (auto elem : items)
    addItem(elem);
}

ItemFactory& ItemFactory::addItem(ItemInfo elem) {
  items.push_back(elem.id);
  weights.push_back(elem.weight);
  CHECK(!elem.count.isEmpty());
  count.push_back(elem.count);
  return *this;
}

ItemFactory& ItemFactory::addUniqueItem(ItemType id, Range count) {
  uniqueCounts.push_back({id, count});
  return *this;
}

ItemFactory& ItemFactory::setRandomPrefixes(double chance) {
  for (auto& item : items)
    item.setPrefixChance(chance);
  return *this;
}

vector<PItem> ItemFactory::random() {
  if (uniqueCounts.size() > 0) {
    ItemType id = uniqueCounts.back().first;
    int cnt = Random.get(uniqueCounts.back().second);
    uniqueCounts.pop_back();
    return id.get(cnt);
  }
  int index = Random.get(weights);
  return items[index].get(Random.get(count[index]));
}

ItemFactory ItemFactory::villageShop() {
  return ItemFactory({
      {ItemType::Scroll{Effect::Teleport{}}, 5 },
      {ItemType::Scroll{Effect::EnhanceArmor{}}, 5 },
      {ItemType::Scroll{Effect::EnhanceWeapon{}}, 5 },
      {ItemType::FireScroll{}, 5 },
      {ItemType::Torch{}, 10 },
      {ItemType::Scroll{Effect::Summon{CreatureId::FIRE_SPHERE, Range(1, 2)}}, 5 },
      {ItemType::Scroll{Effect::CircularBlast{}}, 1 },
      {ItemType::Scroll{Effect::Deception{}}, 2 },
      {ItemType::Scroll{Effect::Summon{CreatureId::FLY, Range(3, 6)}}, 5 },
      {ItemType::Potion{Effect::Heal{}}, 7 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SLEEP}}, 5 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SLOWED}}, 5 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SPEED}},5 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::BLIND}}, 5 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::INVISIBLE}}, 2 },
      {ItemType::Amulet{LastingEffect::WARNING}, 0.5 },
      {ItemType::Amulet{LastingEffect::REGENERATION}, 0.5 },
      {ItemType::DefenseAmulet{}, 0.5 },
      {ItemType::Ring{LastingEffect::POISON_RESISTANT}, 0.5},
      {ItemType::Ring{LastingEffect::FIRE_RESISTANT}, 0.5}})
      .setRandomPrefixes(0.03);
}

ItemFactory ItemFactory::dwarfShop() {
  return armory();
}

ItemFactory ItemFactory::armory() {
  return ItemFactory({
      {ItemType::Knife{}, 5 },
      {ItemType::Sword{}, 2 },
      {ItemType::BattleAxe{}, 2 },
      {ItemType::WarHammer{}, 2 },
      {ItemType::Bow{}, 4 },
      {ItemType::LeatherArmor{}, 2 },
      {ItemType::ChainArmor{}, 1 },
      {ItemType::LeatherHelm{}, 2 },
      {ItemType::IronHelm{}, 1},
      {ItemType::LeatherBoots{}, 2 },
      {ItemType::LeatherGloves{}, 2 },
      {ItemType::IronBoots{}, 1} })
      .addUniqueItem(ItemType::Bow{})
      .setRandomPrefixes(0.03);
}

ItemFactory ItemFactory::gnomeShop() {
  return ItemFactory({
      {ItemType::Knife{}, 5 },
      {ItemType::Sword{}, 2 },
      {ItemType::BattleAxe{}, 1 },
      {ItemType::WarHammer{}, 2 },
      {ItemType::LeatherArmor{}, 2 },
      {ItemType::ChainArmor{}, 1 },
      {ItemType::LeatherHelm{}, 2 },
      {ItemType::IronHelm{}, 1 },
      {ItemType::LeatherBoots{}, 2 },
      {ItemType::IronBoots{}, 1 },
      {ItemType::LeatherGloves{}, 2 },
      {ItemType::Torch{}, 10 }})
      .addUniqueItem({ItemType::AutomatonItem{}})
      .setRandomPrefixes(0.03);
}

ItemFactory ItemFactory::dragonCave() {
  return ItemFactory({
      {ItemType::GoldPiece{}, 10, Range(30, 100) },
      {ItemType::Sword{}, 1 },
      {ItemType::BattleAxe{}, 1 },
      {ItemType::WarHammer{}, 1 }})
      .setRandomPrefixes(1);
}

ItemFactory ItemFactory::minerals() {
  return ItemFactory({
      {ItemType::IronOre{}, 1 },
      {ItemType::Rock{}, 1 }});
}

ItemFactory ItemFactory::potions() {
  return ItemFactory({
      {ItemType::Potion{Effect::Heal{}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SLEEP}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SLOWED}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::BLIND}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::INVISIBLE}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::POISON}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::POISON_RESISTANT}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::FLYING}}, 1 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SPEED}}, 1 }});
}

ItemFactory ItemFactory::scrolls() {
  return ItemFactory({
      {ItemType::Scroll{Effect::Teleport{}}, 1 },
      {ItemType::Scroll{Effect::EnhanceArmor{}}, 1 },
      {ItemType::Scroll{Effect::EnhanceWeapon{}}, 1 },
      {ItemType::FireScroll{}, 1 },
      {ItemType::Scroll{Effect::Summon{CreatureId::FIRE_SPHERE, Range(1, 2)}}, 1 },
      {ItemType::Scroll{Effect::CircularBlast{}}, 1 },
      {ItemType::Scroll{Effect::Deception{}}, 1 },
      {ItemType::Scroll{Effect::Summon{CreatureId::FLY, Range(3, 6)}}, 1 }});
}

ItemFactory ItemFactory::mushrooms(bool onlyGood) {
  return ItemFactory({
      {ItemType::Mushroom{Effect::Lasting{LastingEffect::DAM_BONUS}}, 1 },
      {ItemType::Mushroom{Effect::Lasting{LastingEffect::DEF_BONUS}}, 1 },
      {ItemType::Mushroom{Effect::Lasting{LastingEffect::NIGHT_VISION}}, 1 },
      {ItemType::Mushroom{Effect::Lasting{LastingEffect::PANIC}}, 1 },
      {ItemType::Mushroom{Effect::Lasting{LastingEffect::REGENERATION}}, 1 },
      {ItemType::Mushroom{Effect::Lasting{LastingEffect::HALLU}}, onlyGood ? 0.1 : 8. },
      {ItemType::Mushroom{Effect::Lasting{LastingEffect::RAGE}}, 1 }});
}

ItemFactory ItemFactory::amulets() {
  return ItemFactory({
    {ItemType::Amulet{LastingEffect::REGENERATION}, 1},
    {ItemType::Amulet{LastingEffect::WARNING}, 1},
    {ItemType::DefenseAmulet{}, 1},}
  );
}

ItemFactory ItemFactory::dungeon() {
  return ItemFactory({
      {ItemType::Knife{}, 50 },
      {ItemType::Sword{}, 50 },
      {ItemType::BattleAxe{}, 10 },
      {ItemType::WarHammer{}, 20 },
      {ItemType::LeatherArmor{}, 20 },
      {ItemType::ChainArmor{}, 1 },
      {ItemType::LeatherHelm{}, 20 },
      {ItemType::IronHelm{}, 5 },
      {ItemType::LeatherBoots{}, 20 },
      {ItemType::IronBoots{}, 7 },
      {ItemType::Torch{}, 7 },
      {ItemType::LeatherGloves{}, 30 },
      {ItemType::Scroll{Effect::Teleport{}}, 30 },
      {ItemType::Scroll{Effect::EnhanceArmor{}}, 30 },
      {ItemType::Scroll{Effect::EnhanceWeapon{}}, 30 },
      {ItemType::FireScroll{}, 30 },
      {ItemType::Scroll{Effect::Summon{CreatureId::FIRE_SPHERE, Range(1, 2)}}, 30 },
      {ItemType::Scroll{Effect::CircularBlast{}}, 5 },
      {ItemType::Scroll{Effect::Deception{}}, 10 },
      {ItemType::Scroll{Effect::Summon{CreatureId::FLY, Range(3, 6)}}, 30 },
      {ItemType::Potion{Effect::Heal{}}, 50 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SLEEP}}, 50 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SLOWED}}, 50 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::BLIND}}, 30 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::INVISIBLE}}, 10 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::POISON}}, 20 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::POISON_RESISTANT}}, 20 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::FLYING}}, 20 },
      {ItemType::Potion{Effect::Lasting{LastingEffect::SPEED}}, 50 },
      {ItemType::Amulet{LastingEffect::WARNING}, 3 },
      {ItemType::Amulet{LastingEffect::REGENERATION}, 3 },
      {ItemType::DefenseAmulet{}, 3 },
      {ItemType::Ring{LastingEffect::POISON_RESISTANT}, 3},
      {ItemType::Ring{LastingEffect::FIRE_RESISTANT}, 3}})
      .setRandomPrefixes(0.03);
}

ItemFactory ItemFactory::chest() {
  return dungeon().addItem({ItemType::GoldPiece{}, 300, Range(4, 9)});
}

ItemFactory ItemFactory::singleType(ItemType id, Range count) {
  return ItemFactory({ItemInfo{id, 1, count}});
}

ItemFactory& ItemFactory::operator = (const ItemFactory&) = default;

ItemFactory::ItemFactory(const ItemFactory&) = default;

ItemFactory::~ItemFactory() {}
