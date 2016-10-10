#include "stdafx.h"
#include "furniture_tick.h"
#include "lasting_effect.h"
#include "creature.h"
#include "monster_ai.h"
#include "creature_factory.h"
#include "body.h"
#include "furniture.h"
#include "creature_attributes.h"
#include "game.h"
#include "player_message.h"
#include "movement_type.h"

void FurnitureTick::handle(FurnitureTickType type, Position pos, Furniture* furniture) {
  switch (type) {
    case FurnitureTickType::BED:
      if (Creature* c = pos.getCreature())
        if (c->isAffected(LastingEffect::SLEEP))
          c->heal(0.005);
      break;
    case FurnitureTickType::PIGSTY:
      if (pos.getCreature() || !Random.roll(10) || pos.getPoisonGasAmount() > 0)
        return;
      for (Position v : pos.neighbors8())
        if (v.getCreature() && v.getCreature()->getBody().isMinionFood())
          return;
      if (Random.roll(5)) {
        PCreature pig = CreatureFactory::fromId(CreatureId::PIG, furniture->getTribe(),
            MonsterAIFactory::stayOnFurniture(furniture->getType()));
        if (pos.canEnter(pig.get()))
          pos.addCreature(std::move(pig));
      }
      break;
    case FurnitureTickType::BOULDER_TRAP:
      for (Vec2 direction : Vec2::directions4(Random)) {
        int radius = 4;
        for (int i = 1; i <= radius; ++i) {
          Position curPos = pos.plus(direction * i);
          if (Creature* other = curPos.getCreature()) {
            if (!other->getTribe()->getFriendlyTribes().contains(furniture->getTribe())) {
              if (!other->getAttributes().getSkills().hasDiscrete(SkillId::DISARM_TRAPS)) {
                pos.getGame()->addEvent({EventId::TRAP_TRIGGERED, pos});
                pos.globalMessage(
                    PlayerMessage("The boulder starts rolling.", MessagePriority::CRITICAL),
                    PlayerMessage("You hear a heavy boulder rolling.", MessagePriority::CRITICAL));
                CHECK(!pos.getCreature());
                pos.addCreature(CreatureFactory::getRollingBoulder(furniture->getTribe(), direction), 0);
              } else {
                other->you(MsgType::DISARM_TRAP, "boulder trap");
                pos.getGame()->addEvent({EventId::TRAP_DISARMED, EventInfo::TrapDisarmed{pos, other}});
              }
              pos.removeFurniture(furniture);
              return;
            }
          }
          if (!curPos.canEnterEmpty({MovementTrait::WALK}))
            break;
        }
      }
      break;
  }
}
