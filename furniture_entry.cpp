#include "stdafx.h"
#include "furniture_entry.h"
#include "level.h"
#include "position.h"
#include "creature.h"
#include "creature_attributes.h"
#include "furniture.h"
#include "player_message.h"
#include "game.h"
#include "effect.h"

FurnitureEntry::FurnitureEntry(FurnitureEntry::EntryData d) : entryData(d) {
}

void FurnitureEntry::handle(WFurniture f, WCreature c) {
  apply_visitor(makeVisitor<void>(
      [&](Sokoban) {
        if (c->getAttributes().isBoulder()) {
          Position pos = c->getPosition();
          pos.globalMessage(c->getName().the() + " fills the " + f->getName());
          pos.removeFurniture(f);
          c->dieNoReason(Creature::DropType::NOTHING);
        } else {
          if (!c->isAffected(LastingEffect::FLYING))
            c->you(MsgType::FALL, "into the " + f->getName() + "!");
          else
            c->you(MsgType::ARE, "sucked into the " + f->getName() + "!");
          auto level = c->getPosition().getLevel();
          level->changeLevel(getOnlyElement(level->getAllStairKeys()), c);
        }
      },
      [&](Trap type) {
        auto position = c->getPosition();
        if (c->getGame()->getTribe(f->getTribe())->isEnemy(c)) {
          if (!c->getAttributes().getSkills().hasDiscrete(SkillId::DISARM_TRAPS)) {
            if (!type.alwaysVisible)
              c->you(MsgType::TRIGGER_TRAP, "");
            Effect::applyToCreature(c, type.effect, EffectStrength::NORMAL);
            position.getGame()->addEvent({EventId::TRAP_TRIGGERED, c->getPosition()});
          } else {
            c->you(MsgType::DISARM_TRAP, Effect::getName(type.effect) + " trap");
            position.getGame()->addEvent({EventId::TRAP_DISARMED, EventInfo::TrapDisarmed{c->getPosition(), c}});
          }
          position.removeFurniture(f);
        }
      }
    ), entryData);
}

bool FurnitureEntry::isVisibleTo(WConstFurniture f, WConstCreature c) const {
  return apply_visitor(makeVisitor<bool>(
      [&](Sokoban) {
        return true;
      },
      [&](Trap type) {
        return type.alwaysVisible || !c->getGame()->getTribe(f->getTribe())->isEnemy(c)
            || c->getAttributes().getSkills().hasDiscrete(SkillId::DISARM_TRAPS);
      }
  ), entryData);
}

SERIALIZE_DEF(FurnitureEntry, entryData)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureEntry)
