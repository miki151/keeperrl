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
#include "effect.h"
#include "name_generator.h"
#include "content_factory.h"
#include "item_list.h"
#include "effect_type.h"
#include "item_types.h"

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
    ItemListId items;
    string msgItem;
  };
  optional<ItemInfo> itemInfo;
};

static void useChest(Position pos, WConstFurniture furniture, Creature* c, const ChestInfo& chestInfo) {
  c->secondPerson("You open the " + furniture->getName());
  c->thirdPerson(c->getName().the() + " opens the " + furniture->getName());
  pos.removeFurniture(furniture, pos.getGame()->getContentFactory()->furniture.getFurniture(
      chestInfo.openedType, furniture->getTribe()));
  if (auto creatureInfo = chestInfo.creatureInfo)
    if (creatureInfo->creatureChance > 0 && Random.roll(creatureInfo->creatureChance)) {
      int numSpawned = 0;
      for (int i : Range(creatureInfo->numCreatures))
        if (pos.getLevel()->landCreature({pos}, CreatureGroup(*creatureInfo->creature).random(
            &pos.getGame()->getContentFactory()->getCreatures())))
          ++numSpawned;
      if (numSpawned > 0)
        c->message(creatureInfo->msgCreature);
      return;
    }
  if (auto itemInfo = chestInfo.itemInfo) {
    c->message(itemInfo->msgItem);
    auto itemList = pos.getGame()->getContentFactory()->itemFactory.get(itemInfo->items);
    vector<PItem> items = itemList.random(pos.getGame()->getContentFactory());
    c->getGame()->addEvent(EventInfo::ItemsAppeared{pos, getWeakPointers(items)});
    pos.dropItems(std::move(items));
  }
}

static void desecrate(Position pos, WConstFurniture furniture, Creature* c) {
  c->verb("desecrate", "desecrates", "the "+ furniture->getName());
  pos.removeFurniture(furniture, pos.getGame()->getContentFactory()->furniture.getFurniture(FurnitureType("ALTAR_DES"), furniture->getTribe()));
  switch (Random.get(5)) {
    case 0:
      pos.globalMessage("A streak of magical energy is released");
      c->addPermanentEffect(Random.choose(
          LastingEffect::RAGE,
          LastingEffect::BLIND,
          LastingEffect::PANIC,
          LastingEffect::SPEED,
          LastingEffect::FLYING,
          LastingEffect::SLOWED,
          LastingEffect::INSANITY,
          LastingEffect::COLLAPSED,
          LastingEffect::INVISIBLE,
          LastingEffect::TELEPATHY,
          LastingEffect::MELEE_RESISTANCE,
          LastingEffect::MELEE_VULNERABILITY,
          LastingEffect::MAGIC_RESISTANCE,
          LastingEffect::MAGIC_VULNERABILITY,
          LastingEffect::RANGED_RESISTANCE,
          LastingEffect::RANGED_VULNERABILITY,
          LastingEffect::BAD_BREATH,
          LastingEffect::NIGHT_VISION,
          LastingEffect::PEACEFULNESS));
      break;
    case 1: {
      pos.globalMessage("A streak of magical energy is released");
      auto ef = Random.choose(
          Effect(Effects::IncreaseAttr{ Random.choose(
              AttrType::DAMAGE, AttrType::DEFENSE, AttrType::SPELL_DAMAGE, AttrType::RANGED_DAMAGE),
              Random.choose(-3, -2, -1, 1, 2, 3) }),
          Effect(Effects::Acid{}),
          Effect(Effects::Fire{}),
          Effect(Effects::Lasting { LastingEffect::DAM_BONUS }),
          Effect(Effects::Lasting { LastingEffect::BLIND }),
          Effect(Effects::Lasting { LastingEffect::POISON }),
          Effect(Effects::Lasting { LastingEffect::BLEEDING }),
          Effect(Effects::Lasting { LastingEffect::HALLU })
      );
      ef.apply(pos);
      break;
    }
    case 2: {
      pos.globalMessage(pos.getGame()->getContentFactory()->getCreatures().getNameGenerator()->getNext(NameGeneratorId("DEITY"))
          + " seems to be very angry");
      auto group = CreatureGroup::singleType(TribeId::getMonster(), CreatureId("ANGEL"));
      Effect::summon(pos, group, Random.get(3, 6), none);
      break;
    }
    case 3: {
      c->verb("find", "finds", "some gold coins in the cracks");
      pos.dropItems(ItemType(CustomItemId("GoldPiece")).get(Random.get(50, 100), pos.getGame()->getContentFactory()));
      break;
    }
    case 4: {
      c->verb("find", "finds", "a glyph in the cracks!");
      pos.dropItem(Random.choose(
          ItemType(ItemTypes::Glyph{ { ItemUpgradeType::ARMOR, ItemPrefix( ItemAttrBonus{ AttrType::DAMAGE, 2 } ) } }),
          ItemType(ItemTypes::Glyph{ { ItemUpgradeType::ARMOR, ItemPrefix( ItemAttrBonus{ AttrType::DEFENSE, 2 } ) } }),
          ItemType(ItemTypes::Glyph{ { ItemUpgradeType::ARMOR, ItemPrefix( LastingEffect::TELEPATHY ) } }),
          ItemType(ItemTypes::Glyph{ { ItemUpgradeType::WEAPON,
              ItemPrefix( VictimEffect { 0.3, EffectType(Effects::Lasting{LastingEffect::BLEEDING}) } ) } })
          ).get(pos.getGame()->getContentFactory()));
      break;
    }
  }
}

static void usePortal(Position pos, Creature* c) {
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

static void sitOnThrone(Position pos, WConstFurniture furniture, Creature* c) {
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
    auto tryTeleporting = [&] (Creature* enemy) {
      if (enemy->getPosition().dist8(pos).value_or(4) > 3 || !c->canSee(enemy))
        if (auto landing = pos.getLevel()->getClosestLanding({pos}, enemy)) {
          enemy->getPosition().moveCreature(*landing, true);
          wasTeleported = true;
          enemy->removeEffect(LastingEffect::SLEEP);
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

void FurnitureUsage::handle(FurnitureUsageType type, Position pos, WConstFurniture furniture, Creature* c) {
  CHECK(c != nullptr);
  switch (type) {
    case FurnitureUsageType::CHEST:
      useChest(pos, furniture, c,
          ChestInfo {
              FurnitureType("OPENED_CHEST"),
              ChestInfo::CreatureInfo {
                  CreatureGroup::singleType(TribeId::getPest(), CreatureId("RAT")),
                  10,
                  Random.get(3, 6),
                  "It's full of rats!",
              },
              ChestInfo::ItemInfo {
                  ItemListId("chest"),
                  "There is an item inside"
              }
          });
      break;
    case FurnitureUsageType::COFFIN:
      useChest(pos, furniture, c,
          ChestInfo {
              FurnitureType("OPENED_COFFIN"),
              none,
              ChestInfo::ItemInfo {
                  ItemListId("chest"),
                  "There is a rotting corpse inside. You find an item."
              }
          });
      break;
    case FurnitureUsageType::VAMPIRE_COFFIN:
      useChest(pos, furniture, c,
          ChestInfo {
              FurnitureType("OPENED_COFFIN"),
              ChestInfo::CreatureInfo {
                  CreatureGroup::singleType(TribeId::getMonster(), CreatureId("VAMPIRE_LORD")), 1, 1,
                  "There is a rotting corpse inside. The corpse is alive!"
              },
              none
          });
      break;
    case FurnitureUsageType::FOUNTAIN: {
      c->secondPerson("You drink from the fountain.");
      c->thirdPerson(c->getName().the() + " drinks from the fountain.");
      auto itemList = pos.getGame()->getContentFactory()->itemFactory.get(ItemListId("potions"));
      PItem potion = itemList.random(pos.getGame()->getContentFactory()).getOnlyElement();
      potion->apply(c);
      break;
    }
    case FurnitureUsageType::SLEEP:
      Effects::Lasting{LastingEffect::SLEEP}.applyToCreature(c);
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
    case FurnitureUsageType::DESECRATE:
      desecrate(pos, furniture, c);
      break;
    case FurnitureUsageType::DEMON_RITUAL:
    case FurnitureUsageType::STUDY:
    case FurnitureUsageType::ARCHERY_RANGE:
      break;
  }
}

bool FurnitureUsage::canHandle(FurnitureUsageType type, const Creature* c) {
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
    case FurnitureUsageType::DESECRATE: return "desecrate " + furnitureName;
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
