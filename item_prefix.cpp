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

void applyPrefix(const ContentFactory* factory, const ItemPrefix& prefix, ItemAttributes& attr) {
  if (attr.automatonPart) {
    attr.automatonPart->prefixes.push_back({prefix, getItemName(factory, prefix)});
  } else {
    attr.prefixes.push_back(getItemName(factory, prefix));
    prefix.visit<void>(
        [&](LastingEffect effect) {
          attr.equipedEffect.push_back(effect);
        },
        [&](const AttackerEffect& e) {
          attr.weaponInfo.attackerEffect.push_back(e.effect);
        },
        [&](const VictimEffect& e) {
          attr.weaponInfo.victimEffect.push_back(e);
        },
        [&](ItemAttrBonus bonus) {
          attr.modifiers[bonus.attr] += bonus.value;
        },
        [&](const JoinPrefixes& join) {
          for (auto& elem : join.prefixes)
            applyPrefix(factory, elem, attr);
        },
        [&](const SpellId& spell) {
          attr.equipedAbility.push_back(spell);
        },
        [&](const SpecialAttr& a) {
          attr.specialAttr[a.attr] = make_pair(a.value, a.predicate);
        }
    );
  }
}

void applyPrefixToCreature(const ItemPrefix& prefix, Creature* c) {
  auto applyToIntrinsicAttack = [&] {
    auto& attacks = c->getBody().getIntrinsicAttacks();
    for (auto part : ENUM_ALL(BodyPart))
      if (!attacks[part].empty()) {
        attacks[part].back().item->applyPrefix(prefix, c->getGame()->getContentFactory());
        break;
      }
  };
  prefix.visit<void>(
      [&](LastingEffect effect) {
        c->addPermanentEffect(effect);
      },
      [&](const AttackerEffect& e) {
        applyToIntrinsicAttack();
      },
      [&](const VictimEffect& e) {
        applyToIntrinsicAttack();
      },
      [&](ItemAttrBonus e) {
        c->getAttributes().increaseBaseAttr(e.attr, e.value);
      },
      [&](const JoinPrefixes& join) {
        for (auto& elem : join.prefixes)
          applyPrefixToCreature(elem, c);
      },
      [&](const SpellId& spell) {
        c->getSpellMap().add(*c->getGame()->getContentFactory()->getCreatures().getSpell(spell),
            ExperienceType::MELEE, 0);
      },
      [&](const SpecialAttr& a) {
        c->getAttributes().specialAttr[a.attr].push_back(make_pair(a.value, a.predicate));
      }
  );
}

vector<string> getEffectDescription(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<vector<string>>(
      [&](LastingEffect effect) -> vector<string> {
        return {"grants " + LastingEffects::getName(effect)};
      },
      [&](const AttackerEffect& e) -> vector<string> {
        return {"attacker affected by: " + e.effect.getName(factory)};
      },
      [&](const VictimEffect& e) -> vector<string> {
        return {"victim affected by: " + e.effect.getName(factory) + " (" + toPercentage(e.chance) + " chance)"};
      },
      [&](ItemAttrBonus bonus) -> vector<string> {
        return {"+"_s + toString(bonus.value) + " " + getName(bonus.attr)};
      },
      [&](const JoinPrefixes& join) -> vector<string> {
        vector<string> ret;
        for (auto& e : join.prefixes)
          ret.append(getEffectDescription(factory, e));
        return ret;
      },
      [&](const SpellId& id) -> vector<string> {
        return {"grants "_s + id.data() + " ability"};
      },
      [&](const SpecialAttr& a) -> vector<string> {
        return {toStringWithSign(a.value) + " " + ::getName(a.attr) + " " + a.predicate.getName()};
      }
  );
}

string getItemName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<string>(
      [&](LastingEffect effect) {
        return "of " + LastingEffects::getName(effect);
      },
      [&](const AttackerEffect& e) {
        return "of " + e.effect.getName(factory);
      },
      [&](const VictimEffect& e) {
        return "of " + e.effect.getName(factory);
      },
      [&](ItemAttrBonus bonus) {
        return "of " + getName(bonus.attr);
      },
      [&](const JoinPrefixes& join) {
        return getItemName(factory, join.prefixes.back());
      },
      [&](const SpellId& id) -> string {
        return "of "_s + factory->getCreatures().getSpell(id)->getName(factory);
      },
      [&](const SpecialAttr& a) -> string {
        return a.predicate.getName();
      }
  );
}

string getGlyphName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit<string>(
      [&](const auto&) {
        return ::getItemName(factory, prefix);
      },
      [&](ItemAttrBonus bonus) {
        return "of +"_s + toString(bonus.value) + " " + getName(bonus.attr);
      },
      [&](const JoinPrefixes& join) {
        return getGlyphName(factory, join.prefixes.back());
      }
  );
}
