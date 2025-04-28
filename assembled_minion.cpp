#include "stdafx.h"
#include "assembled_minion.h"
#include "collective.h"
#include "creature_group.h"
#include "creature.h"
#include "equipment.h"
#include "creature_attributes.h"
#include "game.h"
#include "content_factory.h"
#include "item.h"
#include "spell_map.h"
#include "body.h"

void AssembledMinion::assemble(Collective* col, Item* item, Position pos) const {
  auto group = CreatureGroup::singleType(col->getTribeId(), creature);
  auto c = Effect::summon(pos, group, 1, none).getFirstElement();
  if (c) {
    (*c)->getEquipment().removeAllItems(*c);
    col->addCreature(*c, traits);
    auto factory = (*c)->getGame()->getContentFactory();
    item->upgrade((*c)->getAttributes().automatonParts.transform(
        [&](const ItemType& t) { return t.get(factory); }), factory);
    for (auto& e : effects)
      e.apply((*c)->getPosition(), nullptr);
    for (auto& a : (*c)->getAttributes().getAllAttr())
      a.second *= scale;
    for (auto& e : item->getAbility())
      (*c)->getSpellMap().add(*(*c)->getGame()->getContentFactory()->getCreatures().getSpell(e.spell.getId()),
          AttrType("DAMAGE"), 0);
    for (auto& mod : item->getModifierValues())
      (*c)->getAttributes().increaseBaseAttr(mod.first, mod.second);
    for (auto& elem : item->getSpecialModifiers())
      (*c)->getAttributes().specialAttr[elem.first].push_back(elem.second);
    item->onEquip(*c, false);
    auto& attacks = (*c)->getBody().getIntrinsicAttacks();
    auto getRandomPart = [&] {
      vector<Item*> withPrefix;
      vector<Item*> withoutPrefix;
      for (auto part : ENUM_ALL(BodyPart))
        for (auto& a : attacks[part])
          if (a.item->getWeaponInfo().attackerEffect.empty() && a.item->getWeaponInfo().victimEffect.empty())
            withoutPrefix.push_back(a.item.get());
          else
            withPrefix.push_back(a.item.get());
      return Random.choose(!withoutPrefix.empty() ? withoutPrefix : withPrefix);
    };
    for (auto& effect : item->getWeaponInfo().attackerEffect)
      getRandomPart()->applyPrefix(ItemPrefixes::AttackerEffect{effect}, factory);
    for (auto& effect : item->getWeaponInfo().victimEffect)
      getRandomPart()->applyPrefix(effect, factory);
  }
}