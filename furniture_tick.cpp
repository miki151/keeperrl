#include "stdafx.h"
#include "furniture_tick.h"
#include "lasting_effect.h"
#include "creature.h"
#include "monster_ai.h"
#include "creature_factory.h"
#include "body.h"
#include "furniture.h"

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
  }
}
