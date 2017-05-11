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
#include "movement_set.h"

FurnitureEntry::FurnitureEntry(FurnitureEntry::EntryData d) : entryData(d) {
}

void FurnitureEntry::handle(WFurniture f, WCreature c) {
  entryData.match(
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
          level->changeLevel(level->getAllStairKeys().getOnlyElement(), c);
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
      },
      [&](Water) {
        MovementType realMovement = c->getMovementType();
        realMovement.setForced(false);
        if (!f->getMovementSet().canEnter(realMovement)) {
          if (auto holding = c->getHoldingCreature()) {
            c->you(MsgType::ARE, "drowned by " + holding->getName().the());
            c->dieWithAttacker(holding, Creature::DropType::NOTHING);
          } else {
            c->you(MsgType::DROWN, f->getName());
            c->dieWithReason("drowned", Creature::DropType::NOTHING);
          }
        }
      },
      [&](Magma) {
        MovementType realMovement = c->getMovementType();
        realMovement.setForced(false);
        if (!f->getMovementSet().canEnter(realMovement)) {
          c->you(MsgType::BURN, f->getName());
          c->dieWithReason("burned to death", Creature::DropType::NOTHING);
        }
      }
  );
}

bool FurnitureEntry::isVisibleTo(WConstFurniture f, WConstCreature c) const {
  return entryData.visit(
      [&](const Trap& type) {
        return type.alwaysVisible || !c->getGame()->getTribe(f->getTribe())->isEnemy(c)
            || c->getAttributes().getSkills().hasDiscrete(SkillId::DISARM_TRAPS);
      },
      [&](const auto&) {
        return true;
      }
  );
}

SERIALIZE_DEF(FurnitureEntry, entryData)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureEntry)
