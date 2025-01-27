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
#include "model.h"

FurnitureEntry::FurnitureEntry(FurnitureEntry::EntryData d) : entryData(d) {
}

void FurnitureEntry::handle(Furniture* f, Creature* c) {
  PROFILE;
  // Check if this is happening during worldgen in which case we don't continue due to potential crashes
  if (!c->getGame())
    return;
  entryData.match(
      [&](Sokoban) {
        if (c->getAttributes().isBoulder()) {
          Position pos = c->getPosition();
          pos.globalMessage(TSentence("OBJECT_FILLS_HOLE", c->getName().the(), f->getName()));
          pos.removeFurniture(f);
          c->dieNoReason(Creature::DropType::NOTHING);
        } else {
          if (!c->isAffected(LastingEffect::FLYING))
            c->verb(TStringId("YOU_FALL_INTO"), TStringId("FALLS_INTO"), f->getName());
          else
            c->verb(TStringId("YOU_ARE_SUCKED_INTO"), TStringId("IS_SUCKED_INTO"), f->getName());
          auto level = c->getPosition().getLevel();
          level->changeLevel(level->getAllStairKeys().getOnlyElement(), c);
        }
      },
      [&](Trap type) {
        auto position = c->getPosition();
        if (auto game = c->getGame()) // check in case the creature is placed here during level generation
          if (game->getTribe(f->getTribe())->isEnemy(c) && !c->hasAlternativeViewId()) {
            auto layer = f->getLayer();
            if (type.invisible || !c->isAffected(BuffId("DISARM_TRAPS_SKILL"))) {
              if (!type.invisible)
                c->you(MsgType::TRIGGER_TRAP);
              type.effect.apply(position, f->getCreator());
            } else {
              game->addEvent(EventInfo::TrapDisarmed{position, f->getType(), c});
              c->you(MsgType::DISARM_TRAP, f->getName());
            }
            if (position.getFurniture(layer) == f) // f might have been removed by the TrapTrigger effect or something else
              position.removeFurniture(f);
          }
      },
      [&](Water) {
        c->removeEffect(LastingEffect::ON_FIRE);
        MovementType realMovement = c->getMovementType();
        realMovement.setForced(false);
        // Workaround to not kill creatures that are being landed as it causes a crash
        if (!f->getMovementSet().canEnter(realMovement) &&
            c->getPosition().getModel()->getAllCreatures().contains(c)) {
          if (auto holding = c->getHoldingCreature()) {
            c->verb(TStringId("YOU_ARE_DROWNED_BY"), TStringId("IS_DROWNED_BY"), holding->getName().the());
            c->dieWithAttacker(holding, Creature::DropType::ONLY_INVENTORY);
          } else {
            c->you(MsgType::DROWN, f->getName());
            c->dieWithReason(TStringId("DROWNED_DEATH_REASON"), Creature::DropType::ONLY_INVENTORY);
          }
        }
      },
      [&](Magma) {
        MovementType realMovement = c->getMovementType();
        realMovement.setForced(false);
        // Workaround to not kill creatures that are being landed as it causes a crash
        if (!f->getMovementSet().canEnter(realMovement) &&
            c->getPosition().getModel()->getAllCreatures().contains(c)) {
          c->verb(TStringId("SCREAM"), TStringId("SCREAMS"));
          c->you(MsgType::BURN, f->getName());
          c->dieWithReason(TStringId("BURNED_DEATH_REASON"), Creature::DropType::ONLY_INVENTORY);
        }
      },
      [&](const Effect& effect) {
        auto attacker = [&] {
          if (auto c = f->getCreator())
            return c;
          for (auto pos : c->getPosition().getRectangle(Rectangle::centered(30)))
            if (auto c = pos.getCreature())
              if (c->getTribeId() == f->getTribe())
                return c;
          return (Creature*)nullptr;
        };
        effect.apply(c->getPosition(), attacker());
      }
  );
}

bool FurnitureEntry::isVisibleTo(const Furniture* f, const Creature* c) const {
  return entryData.visit(
      [&](const Trap& type) {
        return !c->getGame()->getTribe(f->getTribe())->isEnemy(c)
            || (!type.invisible && c->isAffected(BuffId("DISARM_TRAPS_SKILL")));
      },
      [&](const auto&) {
        return true;
      }
  );
}

SERIALIZE_DEF(FurnitureEntry, entryData)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureEntry)

#include "pretty_archive.h"
template void FurnitureEntry::serialize(PrettyInputArchive&, unsigned);
