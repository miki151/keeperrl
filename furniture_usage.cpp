#include "stdafx.h"
#include "furniture_usage.h"
#include "furniture.h"
#include "creature_factory.h"
#include "furniture_type.h"
#include "tribe.h"
#include "furniture_factory.h"
#include "creature.h"
#include "player_message.h"
#include "creature_group.h"
#include "game.h"
#include "event_listener.h"
#include "item.h"
#include "effect.h"
#include "lasting_effect.h"
#include "creature_name.h"
#include "level.h"
#include "sound.h"
#include "body.h"
#include "creature_attributes.h"
#include "gender.h"
#include "collective.h"
#include "territory.h"
#include "game_event.h"

struct ChestInfo {
  FurnitureType openedType;
  struct CreatureInfo {
    optional<CreatureGroup> creature;
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

static void useChest(Position pos, WConstFurniture furniture, WCreature c, const ChestInfo& chestInfo) {
  c->secondPerson("You open the " + furniture->getName());
  c->thirdPerson(c->getName().the() + " opens the " + furniture->getName());
  pos.removeFurniture(furniture, FurnitureFactory::get(chestInfo.openedType, furniture->getTribe()));
  if (auto creatureInfo = chestInfo.creatureInfo)
    if (creatureInfo->creatureChance > 0 && Random.roll(creatureInfo->creatureChance)) {
      int numSpawned = 0;
      for (int i : Range(creatureInfo->numCreatures))
        if (pos.getLevel()->landCreature({pos}, CreatureGroup(*creatureInfo->creature).random(
            pos.getGame()->getCreatureFactory())))
          ++numSpawned;
      if (numSpawned > 0)
        c->message(creatureInfo->msgCreature);
      return;
    }
  if (auto itemInfo = chestInfo.itemInfo) {
    c->message(itemInfo->msgItem);
    ItemFactory itemFactory(itemInfo->items);
    vector<PItem> items = itemFactory.random();
    c->getGame()->addEvent(EventInfo::ItemsAppeared{c->getPosition(), getWeakPointers(items)});
    c->getPosition().dropItems(std::move(items));
  }
}

static void usePortal(Position pos, WCreature c) {
  c->you(MsgType::ENTER_PORTAL, "");
  if (auto otherPos = pos.getOtherPortal())
    for (auto f : otherPos->getFurniture())
      if (f->getUsageType() == FurnitureUsageType::PORTAL) {
        if (pos.canMoveCreature(*otherPos)) {
          pos.moveCreature(*otherPos, true);
          return;
        }
        for (Position v : otherPos->neighbors8(Random))
          if (pos.canMoveCreature(v)) {
            pos.moveCreature(v, true);
            return;
          }
      }
  c->privateMessage("The portal is inactive. Create another one to open a connection.");
}

static void sitOnThrone(Position pos, WConstFurniture furniture, WCreature c) {
  c->thirdPerson(c->getName().the() + " sits on the " + furniture->getName());
  c->secondPerson("You sit on the " + furniture->getName());
  if (furniture->getTribe() == c->getTribeId())
    c->privateMessage("Frankly, it's not as exciting as it sounds");
  else {
    auto collective = [&]() -> WCollective {
      for (auto col : c->getGame()->getCollectives())
        if (col->getTerritory().contains(pos))
          return col;
      return nullptr;
    }();
    if (!collective)
      return;
    bool wasTeleported = false;
    auto tryTeleporting = [&] (WCreature enemy) {
      if (enemy->getPosition().dist8(pos) > 3 || !c->canSee(enemy))
        if (auto landing = pos.getLevel()->getClosestLanding({pos}, enemy)) {
          enemy->getPosition().moveCreature(*landing, true);
          wasTeleported = true;
        }
    };
    for (auto enemy : collective->getCreatures(MinionTrait::FIGHTER))
      tryTeleporting(enemy);
    if (collective->getLeader())
      tryTeleporting(collective->getLeader());
    if (wasTeleported)
      c->privateMessage(PlayerMessage("Thy audience hath been summoned"_s +
          get(c->getAttributes().getGender(), ", Sire", ", Dame", ""), MessagePriority::HIGH));
    else
      c->privateMessage("Nothing happens");
  }
}

void FurnitureUsage::handle(FurnitureUsageType type, Position pos, WConstFurniture furniture, WCreature c) {
  CHECK(c != nullptr);
  switch (type) {
    case FurnitureUsageType::CHEST:
      useChest(pos, furniture, c,
          ChestInfo {
              FurnitureType::OPENED_CHEST,
              ChestInfo::CreatureInfo {
                  CreatureGroup::singleCreature(TribeId::getPest(), "RAT"),
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
      useChest(pos, furniture, c,
          ChestInfo {
              FurnitureType::OPENED_COFFIN,
              none,
              ChestInfo::ItemInfo {
                  ItemFactory::chest(),
                  "There is a rotting corpse inside. You find an item."
              }
          });
      break;
    case FurnitureUsageType::VAMPIRE_COFFIN:
      useChest(pos, furniture, c,
          ChestInfo {
              FurnitureType::OPENED_COFFIN,
              ChestInfo::CreatureInfo {
                  CreatureGroup::singleCreature(TribeId::getMonster(), "VAMPIRE_LORD"), 1, 1,
                  "There is a rotting corpse inside. The corpse is alive!"
              },
              none
          });
      break;
    case FurnitureUsageType::FOUNTAIN: {
      c->secondPerson("You drink from the fountain.");
      c->thirdPerson(c->getName().the() + " drinks from the fountain.");
      PItem potion = ItemFactory::potions().random().getOnlyElement();
      potion->apply(c);
      break;
    }
    case FurnitureUsageType::SLEEP:
      Effect::Lasting{LastingEffect::SLEEP}.applyToCreature(c);
      break;
    case FurnitureUsageType::KEEPER_BOARD:
      c->getGame()->handleMessageBoard(pos, c);
      break;
    case FurnitureUsageType::CROPS:
      if (Random.roll(3)) {
        c->thirdPerson(c->getName().the() + " scythes the field.");
        c->secondPerson("You scythe the field.");
      }
      break;
    case FurnitureUsageType::STAIRS:
      c->getLevel()->changeLevel(*pos.getLandingLink(), c);
      break;
    case FurnitureUsageType::TIE_UP:
      c->addEffect(LastingEffect::TIED_UP, 100_visible);
      break;
    case FurnitureUsageType::TRAIN:
      c->addSound(SoundId::MISSED_ATTACK);
      break;
    case FurnitureUsageType::PORTAL:
      usePortal(pos, c);
      break;
    case FurnitureUsageType::SIT_ON_THRONE:
      sitOnThrone(pos, furniture, c);
      break;
    case FurnitureUsageType::STUDY:
    case FurnitureUsageType::ARCHERY_RANGE:
      break;
  }
}

bool FurnitureUsage::canHandle(FurnitureUsageType type, WConstCreature c) {
  switch (type) {
    case FurnitureUsageType::KEEPER_BOARD:
    case FurnitureUsageType::FOUNTAIN:
    case FurnitureUsageType::VAMPIRE_COFFIN:
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
    case FurnitureUsageType::SLEEP: return "sleep in " + furnitureName;
    case FurnitureUsageType::KEEPER_BOARD: return "view " + furnitureName;
    case FurnitureUsageType::PORTAL: return "enter " + furnitureName;
    case FurnitureUsageType::SIT_ON_THRONE: return "sit on " + furnitureName;
    default: break;
  }
  return "";
}

void FurnitureUsage::beforeRemoved(FurnitureUsageType type, Position pos) {
  switch (type) {
    case FurnitureUsageType::PORTAL:
      pos.removePortal();
      break;
    default:
      break;
  }
}
