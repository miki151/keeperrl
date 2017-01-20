#include "stdafx.h"
#include "furniture_usage.h"
#include "furniture.h"
#include "creature_factory.h"
#include "furniture_type.h"
#include "tribe.h"
#include "furniture_factory.h"
#include "creature.h"
#include "player_message.h"
#include "creature_factory.h"
#include "game.h"
#include "event_listener.h"
#include "item.h"
#include "effect.h"
#include "effect_type.h"
#include "lasting_effect.h"
#include "creature_name.h"
#include "level.h"
#include "sound.h"
#include "body.h"

struct ChestInfo {
  FurnitureType openedType;
  struct CreatureInfo {
    optional<CreatureFactory> creature;
    int creatureChance;
    int numCreatures;
    string msgCreature;
  };
  optional<CreatureInfo> creatureInfo;
  struct ItemInfo {
    ItemFactory items;
    string msgItem;
  };
  optional<ItemInfo> itemInfo;
};

static void useChest(Position pos, const Furniture* furniture, Creature* c, const ChestInfo& chestInfo) {
  c->playerMessage("You open the " + furniture->getName());
  pos.replaceFurniture(furniture, FurnitureFactory::get(chestInfo.openedType, furniture->getTribe()));
  if (auto creatureInfo = chestInfo.creatureInfo)
    if (creatureInfo->creatureChance > 0 && Random.roll(creatureInfo->creatureChance)) {
    int numR = creatureInfo->numCreatures;
    CreatureFactory factory(*creatureInfo->creature);
    for (Position v : c->getPosition().neighbors8(Random)) {
      PCreature rat = factory.random();
      if (v.canEnter(rat.get())) {
        v.addCreature(std::move(rat));
        if (--numR == 0)
          break;
      }
    }
    if (numR < creatureInfo->numCreatures)
      c->playerMessage(creatureInfo->msgCreature);
    return;
  }
  if (auto itemInfo = chestInfo.itemInfo) {
    c->playerMessage(itemInfo->msgItem);
    ItemFactory itemFactory(itemInfo->items);
    vector<PItem> items = itemFactory.random();
    c->getGame()->addEvent({EventId::ITEMS_APPEARED, EventInfo::ItemsAppeared{c->getPosition(),
        extractRefs(items)}});
    c->getPosition().dropItems(std::move(items));
  }
}

void FurnitureUsage::handle(FurnitureUsageType type, Position pos, const Furniture* furniture, Creature* c) {
  CHECK(c != nullptr);
  switch (type) {
    case FurnitureUsageType::CHEST:
      useChest(pos, furniture, c, ChestInfo {
                 FurnitureType::OPENED_CHEST,
                 ChestInfo::CreatureInfo {
                   CreatureFactory::singleCreature(TribeId::getPest(), CreatureId::RAT),
                   10,
                   Random.get(3, 6),
                   "It's full of rats!",
                 },
                 ChestInfo::ItemInfo {
                   ItemFactory::chest(),
                   "There is an item inside"
                 }
               });
      break;
    case FurnitureUsageType::COFFIN:
      useChest(pos, furniture, c, ChestInfo {
                 FurnitureType::OPENED_COFFIN,
                 none,
                 ChestInfo::ItemInfo {
                   ItemFactory::chest(),
                   "There is a rotting corpse inside. You find an item."
                 }
               });
      break;
    case FurnitureUsageType::VAMPIRE_COFFIN:
      useChest(pos, furniture, c, ChestInfo{
                 FurnitureType::OPENED_CHEST,
                 ChestInfo::CreatureInfo {
                   CreatureFactory::singleCreature(TribeId::getKeeper(), CreatureId::VAMPIRE_LORD), 1, 1,
                   "There is a rotting corpse inside. The corpse is alive!"
                 },
                 none
               });
      break;
    case FurnitureUsageType::FOUNTAIN: {
      c->playerMessage("You drink from the fountain.");
      PItem potion = getOnlyElement(ItemFactory::potions().random());
      potion->apply(c);
      break;
    }
    case FurnitureUsageType::SLEEP:
      Effect::applyToCreature(c, {EffectId::LASTING, LastingEffect::SLEEP}, EffectStrength::STRONG);
      break;
    case FurnitureUsageType::KEEPER_BOARD:
      c->getGame()->handleMessageBoard(pos, c);
      break;
    case FurnitureUsageType::CROPS:
      if (Random.roll(3))
        c->globalMessage(c->getName().the() + " scythes the field.");
      break;
    case FurnitureUsageType::STAIRS:
      c->getLevel()->changeLevel(*pos.getLandingLink(), c);
      break;
    case FurnitureUsageType::TIE_UP:
      c->addEffect(LastingEffect::TIED_UP, 100);
      break;
    case FurnitureUsageType::TRAIN:
      c->addSound(SoundId::MISSED_ATTACK);
      break;
    default: break;
  }
}

bool FurnitureUsage::canHandle(FurnitureUsageType type, const Creature* c) {
  switch (type) {
    case FurnitureUsageType::KEEPER_BOARD:
    case FurnitureUsageType::FOUNTAIN:
    case FurnitureUsageType::COFFIN:
    case FurnitureUsageType::CHEST:
      return c->getBody().isHumanoid();
    default:
      return true;
  }
}

string FurnitureUsage::getUsageQuestion(FurnitureUsageType type, string furnitureName) {
    switch (type) {
        case FurnitureUsageType::STAIRS: return "use " + furnitureName;
        case FurnitureUsageType::COFFIN:
        case FurnitureUsageType::VAMPIRE_COFFIN:
        case FurnitureUsageType::CHEST: return "open " + furnitureName;
        case FurnitureUsageType::FOUNTAIN: return "drink from " + furnitureName;
        case FurnitureUsageType::SLEEP: return "sleep on " + furnitureName;
        case FurnitureUsageType::KEEPER_BOARD: return "view " + furnitureName;
        default: break;
    }
    return "";
}
