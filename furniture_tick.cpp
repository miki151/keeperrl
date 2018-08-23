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

static void handleBed(Position pos) {
  PROFILE;
  if (WCreature c = pos.getCreature())
    if (c->isAffected(LastingEffect::SLEEP))
      c->heal(0.005);
}

static void handlePigsty(Position pos, WFurniture furniture) {
  PROFILE;
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

static void handleBoulder(Position pos, WFurniture furniture) {
  for (Vec2 direction : Vec2::directions4(Random)) {
    int radius = 4;
    for (int i = 1; i <= radius; ++i) {
      Position curPos = pos.plus(direction * i);
      if (WCreature other = curPos.getCreature()) {
        if (!other->getTribe()->getFriendlyTribes().contains(furniture->getTribe())) {
          if (!other->getAttributes().getSkills().hasDiscrete(SkillId::DISARM_TRAPS)) {
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

static void meteorShower(Position position, WFurniture furniture) {
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
        ItemType(ItemType::Rock{}).get(),
        Attack(furniture->getCreator(), AttackLevel::MIDDLE, AttackType::HIT, 25, AttrType::DAMAGE),
        10,
        -direction,
        VisionId::NORMAL);
    break;
  }
}

static void pit(Position position, WFurniture self) {
  if (!position.getCreature() && Random.roll(10))
    for (auto neighborPos : position.neighbors8(Random))
      if (auto water = neighborPos.getFurniture(FurnitureLayer::GROUND))
        if (water->canBuildBridgeOver()) {
          position.removeFurniture(position.getFurniture(FurnitureLayer::GROUND),
              FurnitureFactory::get(water->getType(), water->getTribe()));
          self->destroy(position, DestroyAction::Type::BOULDER);
          return;
        }
}

void FurnitureTick::handle(FurnitureTickType type, Position pos, WFurniture furniture) {
  switch (type) {
    case FurnitureTickType::BED:
      handleBed(pos);
      break;
    case FurnitureTickType::PIGSTY:
      handlePigsty(pos, furniture);
      break;
    case FurnitureTickType::BOULDER_TRAP:
      handleBoulder(pos, furniture);
      break;
    case FurnitureTickType::PORTAL:
      pos.registerPortal();
      furniture->getViewObject()->setPortalVersion(none);
      if (auto otherPos = pos.getOtherPortal())
        for (auto f : otherPos->modFurniture())
          if (f->getUsageType() == FurnitureUsageType::PORTAL) {
            auto color = *pos.getPortalIndex();
            furniture->getViewObject()->setPortalVersion(uint8_t(color));
            f->getViewObject()->setPortalVersion(uint8_t(color));
            pos.setNeedsRenderUpdate(true);
            otherPos->setNeedsRenderUpdate(true);
          }
      break;
    case FurnitureTickType::METEOR_SHOWER:
      meteorShower(pos, furniture);
      break;
    case FurnitureTickType::PIT:
      pit(pos, furniture);
      break;
  }
}
