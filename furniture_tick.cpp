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
#include "attack_level.h"
#include "attack_type.h"
#include "item_type.h"
#include "attack.h"
#include "tribe.h"
#include "furniture_type.h"
#include "furniture_factory.h"
#include "view_object.h"
#include "furniture_usage.h"
#include "game_event.h"
#include "color.h"
#include "content_factory.h"
#include "item_types.h"

static void handleBed(Position pos) {
  PROFILE;
  if (Creature* c = pos.getCreature())
    if (c->isAffected(LastingEffect::SLEEP))
      c->heal(0.005);
}

static void handlePigsty(Position pos, Furniture* furniture) {
  PROFILE;
  if (pos.getCreature() || !Random.roll(10) || pos.getPoisonGasAmount() > 0)
    return;
  for (Position v : pos.neighbors8())
    if (v.getCreature() && v.getCreature()->getBody().isMinionFood())
      return;
  if (Random.roll(5)) {
    PCreature pig = pos.getGame()->getContentFactory()->getCreatures().fromId(CreatureId("PIG"), furniture->getTribe(),
        MonsterAIFactory::stayOnFurniture(furniture->getType()));
    if (pos.canEnter(pig.get()))
      pos.addCreature(std::move(pig));
  }
}

static void handleBoulder(Position pos, Furniture* furniture) {
  for (Vec2 direction : Vec2::directions4(Random)) {
    int radius = 4;
    for (int i = 1; i <= radius; ++i) {
      Position curPos = pos.plus(direction * i);
      if (Creature* other = curPos.getCreature()) {
        if (!other->getTribe()->getFriendlyTribes().contains(furniture->getTribe())) {
          if (!other->isAffected(LastingEffect::DISARM_TRAPS_SKILL)) {
            pos.getGame()->addEvent(EventInfo::TrapTriggered{pos});
            pos.globalMessage(PlayerMessage("The boulder starts rolling.", MessagePriority::CRITICAL));
            pos.unseenMessage(PlayerMessage("You hear a heavy boulder rolling.", MessagePriority::CRITICAL));
            CHECK(!pos.getCreature());
            pos.addCreature(CreatureFactory::getRollingBoulder(TribeId::getHostile(), direction), 0_visible);
          } else {
            other->you(MsgType::DISARM_TRAP, "boulder trap");
            pos.getGame()->addEvent(EventInfo::TrapDisarmed{pos, other});
          }
          pos.removeFurniture(furniture);
          return;
        }
      }
      if (!curPos.canEnterEmpty({MovementTrait::WALK}))
        break;
    }
  }
}

static void meteorShower(Position position, Furniture* furniture) {
  auto creator = furniture->getCreator();
  const auto duration = 15_visible;
  if (!creator ||
      creator->isDead() ||
      *furniture->getCreatedTime() + duration < position.getModel()->getLocalTime()) {
    position.removeFurniture(furniture);
    return;
  }
  const int areaWidth = 3;
  const int range = 4;
  for (int i : Range(10)) {
    Position targetPoint = position.plus(Vec2(Random.get(-areaWidth / 2, areaWidth / 2 + 1),
                     Random.get(-areaWidth / 2, areaWidth / 2 + 1)));
    Vec2 direction(Random.get(-1, 2), Random.get(-1, 2));
    if (!targetPoint.isValid() || direction.length8() == 0)
      continue;
    for (int i : Range(range + 1))
      if (!targetPoint.plus(direction * i).canEnter(MovementType({MovementTrait::WALK, MovementTrait::FLY})))
        continue;
    targetPoint.plus(direction * range).throwItem(
        makeVec(ItemType(CustomItemId("Rock")).get(position.getGame()->getContentFactory())),
        Attack(furniture->getCreator(), AttackLevel::MIDDLE, AttackType::HIT, 25, AttrType::DAMAGE),
        10,
        position.minus(direction),
        VisionId::NORMAL);
    break;
  }
}

static void pit(Position position, Furniture* self) {
  if (!position.getCreature() && Random.roll(10))
    for (auto neighborPos : position.neighbors8(Random))
      if (auto water = neighborPos.getFurniture(FurnitureLayer::GROUND))
        if (auto fillType = water->getFillPit()) {
          auto toAdd = position.getGame()->getContentFactory()->furniture.getFurniture(*fillType, water->getTribe());
          if (water->getViewObject()->hasModifier(ViewObjectModifier::BLOODY))
            toAdd->spreadBlood(position);
          position.removeFurniture(position.getFurniture(FurnitureLayer::GROUND), std::move(toAdd));
          self->destroy(position, DestroyAction::Type::BOULDER);
          return;
        }
}

static Color getPortalColor(int index) {
  CHECK(index >= 0);
  index += 1 + 2 * (index / 6);
  return Color(255 * (index % 2), 255 * ((index / 2) % 2), 255 * ((index / 4) % 2));
}

void FurnitureTick::handle(FurnitureTickType type, Position pos, Furniture* furniture) {
  type.visit(
      [&](BuiltinTickType t) {
        switch (t) {
          case BuiltinTickType::BED:
            handleBed(pos);
            break;
          case BuiltinTickType::PIGSTY:
            handlePigsty(pos, furniture);
            break;
          case BuiltinTickType::BOULDER_TRAP:
            handleBoulder(pos, furniture);
            break;
          case BuiltinTickType::PORTAL:
            pos.registerPortal();
            furniture->getViewObject()->setColorVariant(Color::WHITE);
            if (auto otherPos = pos.getOtherPortal())
              for (auto f : otherPos->modFurniture())
                if (f->hasUsageType(BuiltinUsageId::PORTAL)) {
                  auto color = getPortalColor(*pos.getPortalIndex());
                  furniture->getViewObject()->setColorVariant(color);
                  f->getViewObject()->setColorVariant(color);
                  pos.setNeedsRenderAndMemoryUpdate(true);
                  otherPos->setNeedsRenderAndMemoryUpdate(true);
                }
            break;
          case BuiltinTickType::METEOR_SHOWER:
            meteorShower(pos, furniture);
            break;
          case BuiltinTickType::PIT:
            pit(pos, furniture);
            break;
          case BuiltinTickType::EXTINGUISH_FIRE:
            if (auto c = pos.getCreature())
              c->removeEffect(LastingEffect::ON_FIRE);
            break;
          case BuiltinTickType::SET_FURNITURE_ON_FIRE: {
            auto handle = [] (const Position& pos) {
              for (auto& f : pos.getFurniture())
                if (f->getFire())
                  pos.modFurniture(f->getLayer())->fireDamage(pos, true);
            };
            for (auto& v : pos.neighbors8())
              if (Random.roll(30))
                handle(v);
            if (Random.roll(10))
              handle(pos);
          }
        }
      },
      [&](const Effect& e) {
        e.apply(pos, furniture->getCreator());
      }
  );
}
