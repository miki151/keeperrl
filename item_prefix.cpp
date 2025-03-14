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

vector<TString> getEffectDescription(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<vector<TString>>(
      [&](const ItemPrefixes::Prefix& a) {
        return ::getEffectDescription(factory, *a.prefix);
      },
      [&](const ItemPrefixes::Suffix& a) {
        return ::getEffectDescription(factory, *a.prefix);
      },
      [&](LastingOrBuff effect) -> vector<TString> {
        return {TSentence("GRANTS_BUFF", getName(effect, factory))};
      },
      [&](const ItemPrefixes::AttackerEffect& e) -> vector<TString> {
        return {TSentence("ATTACKER_AFFECTED_BY", e.effect.getName(factory))};
      },
      [&](const ItemPrefixes::VictimEffect& e) -> vector<TString> {
        return {TSentence("VICTIM_AFFECTED_BY", e.effect.getName(factory), toPercentage(e.chance))};
      },
      [&](ItemPrefixes::ItemAttrBonus bonus) -> vector<TString> {
        return {TSentence("PLUS_MINUS_ATTR", TString("+"_s + toString(bonus.value)), factory->attrInfo.at(bonus.attr).name)};
      },
      [&](const ItemPrefixes::JoinPrefixes& join) -> vector<TString> {
        vector<TString> ret;
        for (auto& e : join.prefixes)
          ret.append(getEffectDescription(factory, e));
        return ret;
      },
      [&](const SpellId& id) -> vector<TString> {
        return {TSentence("GRANTS_ABILITY", factory->getCreatures().getSpell(id)->getName(factory))};
      },
      [&](const ItemPrefixes::Scale& e) -> vector<TString> {
        return {TSentence("ITEM_ATTR_IMPROVED_BY", TString(e.value))};
      },
      [&](const SpecialAttr& a) -> vector<TString> {
        return {TSentence("SPECIAL_ATTR_VALUE", {toStringWithSign(a.value), factory->attrInfo.at(a.attr).name,
            a.predicate.getName(factory)})};
      },
      [&](const ItemPrefixes::AssembledCreatureEffect& effect) -> vector<TString> {
        return {effect.getDescription(factory)};
      }
  );
}

optional<TString> getItemName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<optional<TString>>(
      [&](LastingOrBuff effect) -> TString {
        return TSentence("OF_SUFFIX", getName(effect, factory));
      },
      [&](const ItemPrefixes::AttackerEffect& e) -> TString {
        return TSentence("OF_SUFFIX", e.effect.getName(factory));
      },
      [&](const ItemPrefixes::VictimEffect& e) -> TString {
        return TSentence("OF_SUFFIX", e.effect.getName(factory));
      },
      [&](ItemPrefixes::ItemAttrBonus bonus) -> TString {
        return TSentence("OF_SUFFIX", factory->attrInfo.at(bonus.attr).name);
      },
      [&](const ItemPrefixes::JoinPrefixes& join) {
        return getItemName(factory, join.prefixes.back());
      },
      [&](const SpellId& id) -> TString {
        return TSentence("OF_SUFFIX", factory->getCreatures().getSpell(id)->getName(factory));
      },
      [&](const SpecialAttr& a) -> TString {
        return a.predicate.getName(factory);
      },
      [&](const auto&) -> optional<TString> {
        return none;
      },
      [&](const ItemPrefixes::AssembledCreatureEffect& effect) -> TString {
        return TSentence("WITH_SUFFIX", effect.getName(factory));
      }
  );
}

TString getGlyphName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<TString>(
      [&](const ItemPrefixes::Prefix& a) {
        return ::getGlyphName(factory, *a.prefix);
      },
      [&](const ItemPrefixes::Suffix& a) {
        return ::getGlyphName(factory, *a.prefix);
      },
      [&](const ItemPrefixes::Scale& a) {
        return TString();
      },
      [&](const auto&) {
        return *::getItemName(factory, prefix);
      },
      [&](ItemPrefixes::ItemAttrBonus bonus) {
        return TSentence("OF_SUFFIX", TSentence("PLUS_MINUS_ATTR", TString("+" + toString(bonus.value)),
            factory->attrInfo.at(bonus.attr).name));
      },
      [&](const ItemPrefixes::JoinPrefixes& join) {
        return getGlyphName(factory, join.prefixes.back());
      }
  );
}
