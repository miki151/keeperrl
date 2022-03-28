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


static void handle(const FurnitureTickTypes::Trap& trap, Position pos, Furniture* furniture) {
  auto dirs = Vec2::directions4();
  for (int dirInd : All(dirs)) {
    for (int i = 1; i <= trap.maxDistance; ++i) {
      Position curPos = pos.plus(dirs[dirInd] * i);
      if (Creature* other = curPos.getCreature()) {
        if (!other->getTribe()->getFriendlyTribes().contains(furniture->getTribe())) {
          if (!other->isAffected(LastingEffect::DISARM_TRAPS_SKILL)) {
            pos.removeFurniture(furniture);
            trap.effects[dirInd].apply(pos, furniture->getCreator());
          } else {
            other->you(MsgType::DISARM_TRAP, furniture->getName());
            pos.getGame()->addEvent(EventInfo::TrapDisarmed{pos, furniture->getType(), other});
            pos.removeFurniture(furniture);
          }
          return;
        }
      }
      if (!curPos.canEnterEmpty({MovementTrait::WALK}))
        break;
    }
  }
}

static void handle(const FurnitureTickTypes::MeteorShower, Position position, Furniture* furniture) {
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
        Attack(furniture->getCreator(), AttackLevel::MIDDLE, AttackType::HIT, 25, AttrType("DAMAGE")),
        10,
        position.minus(direction),
        VisionId::NORMAL);
    break;
  }
}

static void handle(const FurnitureTickTypes::Pit, Position position, Furniture* self) {
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

static void handle(const FurnitureTickTypes::SetFurnitureOnFire, Position pos, Furniture* furniture) {
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

static void handle(const Effect& effect, Position pos, Furniture* furniture) {
  effect.apply(pos, furniture->getCreator());
}

void FurnitureTickType::handle(Position pos, Furniture* furniture) {
  visit<void>([&](const auto& p) { return ::handle(p, pos, furniture); });
}
