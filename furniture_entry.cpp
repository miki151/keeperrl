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
#include "game_event.h"

FurnitureEntry::FurnitureEntry(FurnitureEntry::EntryData d) : entryData(d) {
}

void FurnitureEntry::handle(WFurniture f, Creature* c) {
  PROFILE;
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
        if (auto game = c->getGame()) // check in case the creature is placed here during level generation
          if (game->getTribe(f->getTribe())->isEnemy(c)) {
            if (type.invisible || !c->isAffected(LastingEffect::DISARM_TRAPS_SKILL)) {
              if (!type.invisible)
                c->you(MsgType::TRIGGER_TRAP, "");
              type.effect.applyToCreature(c);
              position.getGame()->addEvent(EventInfo::TrapTriggered{c->getPosition()});
            } else {
              c->you(MsgType::DISARM_TRAP, type.effect.getName() + " trap");
              position.getGame()->addEvent(EventInfo::TrapDisarmed{c->getPosition(), c});
            }
            position.removeFurniture(f);
          }
      },
      [&](Water) {
        c->removeEffect(LastingEffect::ON_FIRE);
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

bool FurnitureEntry::isVisibleTo(WConstFurniture f, const Creature* c) const {
  return entryData.visit(
      [&](const Trap& type) {
        return !c->getGame()->getTribe(f->getTribe())->isEnemy(c)
            || (!type.invisible && c->isAffected(LastingEffect::DISARM_TRAPS_SKILL));
      },
      [&](const auto&) {
        return true;
      }
  );
}

SERIALIZE_DEF(FurnitureEntry, entryData)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureEntry)
