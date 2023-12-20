#include "stdafx.h"
#include "item_prefix.h"
#include "item_attributes.h"
#include "lasting_effect.h"
#include "effect.h"
#include "creature_factory.h"
#include "content_factory.h"
#include "creature.h"
#include "creature_attributes.h"
#include "spell_map.h"
#include "game.h"
#include "content_factory.h"
#include "body.h"
#include "item.h"
#include "effect_type.h"

void applyPrefix(const ContentFactory* factory, const ItemPrefix& prefix, ItemAttributes& attr) {
  if (auto name = getItemName(factory, prefix))
    attr.suffixes.push_back(*name);
  prefix.visit<void>(
      [&](LastingOrBuff effect) {
        attr.equipedEffect.push_back(effect);
      },
      [&](const ItemPrefixes::AttackerEffect& e) {
        attr.weaponInfo.attackerEffect.push_back(e.effect);
      },
      [&](const ItemPrefixes::VictimEffect& e) {
        attr.weaponInfo.victimEffect.push_back(e);
      },
      [&](ItemPrefixes::ItemAttrBonus bonus) {
        attr.modifiers[bonus.attr] += bonus.value;
      },
      [&](const ItemPrefixes::JoinPrefixes& join) {
        for (auto& elem : join.prefixes)
          applyPrefix(factory, elem, attr);
      },
      [&](const SpellId& spell) {
        attr.equipedAbility.push_back(spell);
      },
      [&](const SpecialAttr& a) {
        attr.specialAttr[a.attr] = make_pair(a.value, a.predicate);
      },
      [&](const ItemPrefixes::Scale& a) {
        attr.scale(a.value, factory);
      },
      [&](const ItemPrefixes::Prefix& a) {
        attr.prefixes.push_back(a.value);
        ::applyPrefix(factory, *a.prefix, attr);
      },
      [&](const ItemPrefixes::Suffix& a) {
        attr.suffixes.push_back(a.value);
        ::applyPrefix(factory, *a.prefix, attr);
      },
      [&](const ItemPrefixes::AssembledCreatureEffect& a) {
        if (attr.assembledMinion)
          attr.assembledMinion->effects.push_back(a);
      }
  );
}

vector<string> getEffectDescription(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<vector<string>>(
      [&](const ItemPrefixes::Prefix& a) {
        return ::getEffectDescription(factory, *a.prefix);
      },
      [&](const ItemPrefixes::Suffix& a) {
        return ::getEffectDescription(factory, *a.prefix);
      },
      [&](LastingOrBuff effect) -> vector<string> {
        return {"grants " + getName(effect, factory)};
      },
      [&](const ItemPrefixes::AttackerEffect& e) -> vector<string> {
        return {"attacker affected by: " + e.effect.getName(factory)};
      },
      [&](const ItemPrefixes::VictimEffect& e) -> vector<string> {
        return {"victim affected by: " + e.effect.getName(factory) + " (" + toPercentage(e.chance) + " chance)"};
      },
      [&](ItemPrefixes::ItemAttrBonus bonus) -> vector<string> {
        return {"+"_s + toString(bonus.value) + " " + factory->attrInfo.at(bonus.attr).name};
      },
      [&](const ItemPrefixes::JoinPrefixes& join) -> vector<string> {
        vector<string> ret;
        for (auto& e : join.prefixes)
          ret.append(getEffectDescription(factory, e));
        return ret;
      },
      [&](const SpellId& id) -> vector<string> {
        return {"grants "_s + id.data() + " ability"};
      },
      [&](const ItemPrefixes::Scale& e) -> vector<string> {
        return {"item attributes are improved by " + toString(e.value)};
      },
      [&](const SpecialAttr& a) -> vector<string> {
        return {toStringWithSign(a.value) + " " + factory->attrInfo.at(a.attr).name + " " +
            a.predicate.getName(factory)};
      },
      [&](const ItemPrefixes::AssembledCreatureEffect& effect) -> vector<string> {
        return {effect.getDescription(factory)};
      }
  );
}

optional<string> getItemName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<optional<string>>(
      [&](LastingOrBuff effect) {
        return "of " + getName(effect, factory);
      },
      [&](const ItemPrefixes::AttackerEffect& e) {
        return "of " + e.effect.getName(factory);
      },
      [&](const ItemPrefixes::VictimEffect& e) {
        return "of " + e.effect.getName(factory);
      },
      [&](ItemPrefixes::ItemAttrBonus bonus) {
        return "of " + factory->attrInfo.at(bonus.attr).name;
      },
      [&](const ItemPrefixes::JoinPrefixes& join) {
        return getItemName(factory, join.prefixes.back());
      },
      [&](const SpellId& id) -> string {
        return "of "_s + factory->getCreatures().getSpell(id)->getName(factory);
      },
      [&](const SpecialAttr& a) -> string {
        return a.predicate.getName(factory);
      },
      [&](const auto&) -> optional<string> {
        return none;
      },
      [&](const ItemPrefixes::AssembledCreatureEffect& effect) -> string {
        return "with " + effect.getName(factory);
      }
  );
}

string getGlyphName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<string>(
      [&](const ItemPrefixes::Prefix& a) {
        return ::getGlyphName(factory, *a.prefix);
      },
      [&](const ItemPrefixes::Suffix& a) {
        return ::getGlyphName(factory, *a.prefix);
      },
      [&](const ItemPrefixes::Scale& a) -> string {
        return "improvement";
      },
      [&](const auto&) {
        return *::getItemName(factory, prefix);
      },
      [&](ItemPrefixes::ItemAttrBonus bonus) {
        return "of +"_s + toString(bonus.value) + " " + factory->attrInfo.at(bonus.attr).name;
      },
      [&](const ItemPrefixes::JoinPrefixes& join) {
        return getGlyphName(factory, join.prefixes.back());
      }
  );
}
