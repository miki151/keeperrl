#include "stdafx.h"
#include "model_builder.h"
#include "level.h"
#include "tribe.h"
#include "inventory.h"
#include "collective_builder.h"
#include "options.h"
#include "player_control.h"
#include "spectator.h"
#include "creature.h"
#include "square.h"
#include "progress_meter.h"
#include "collective.h"
#include "level_maker.h"
#include "model.h"
#include "level_builder.h"
#include "monster_ai.h"
#include "game.h"
#include "campaign.h"
#include "creature_name.h"
#include "villain_type.h"
#include "enemy_factory.h"
#include "view_object.h"
#include "item.h"
#include "furniture.h"
#include "sokoban_input.h"
#include "external_enemies.h"
#include "immigration.h"
#include "technology.h"
#include "keybinding.h"
#include "tutorial.h"
#include "settlement_info.h"
#include "enemy_info.h"
#include "game_time.h"
#include "lasting_effect.h"
#include "skill.h"

using namespace std::chrono;

ModelBuilder::ModelBuilder(ProgressMeter* m, RandomGen& r, Options* o, SokobanInput* sok) : random(r), meter(m), options(o),
  enemyFactory(EnemyFactory(random)), sokobanInput(sok) {
}

ModelBuilder::~ModelBuilder() {
}

static vector<ImmigrantInfo> getDarkKeeperImmigration(RandomGen& random) {
  return {
    ImmigrantInfo(CreatureId::GOBLIN, {MinionTrait::FIGHTER, MinionTrait::NO_AUTO_EQUIPMENT})
        .setFrequency(0.7)
        .addRequirement(0.1, AttractionInfo{1, vector<AttractionType>(
             {FurnitureType::FORGE, FurnitureType::WORKSHOP, FurnitureType::JEWELER})})
        .addSpecialTrait(0.03, {SkillId::WORKSHOP, LastingEffect::INSANITY})
        .addSpecialTrait(0.03, {SkillId::FORGE, LastingEffect::INSANITY})
        .addSpecialTrait(0.03, {SkillId::JEWELER, LastingEffect::INSANITY}),
    ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER})
        .setFrequency(0.7)
        .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_WOOD})
        .addSpecialTrait(0.05, ExtraTraining { ExperienceType::ARCHERY, 2 })
        .addSpecialTrait(0.05, ExtraTraining { ExperienceType::MELEE, 2 })
        .addSpecialTrait(0.03, {AttrBonus { AttrType::DAMAGE, 7 }, LastingEffect::INSANITY})
        .addSpecialTrait(0.05, LastingEffect::NIGHT_VISION)
        .addSpecialTrait(0.05, SkillId::DISARM_TRAPS)
        .addSpecialTrait(0.05, SkillId::SWIMMING)
        .addOneOrMoreTraits(0.05, {LastingEffect::HATE_ELVES, LastingEffect::HATE_HUMANS})
        .addSpecialTrait(0.05, LastingEffect::MAGIC_VULNERABILITY),
    ImmigrantInfo(CreatureId::ORC_SHAMAN, {MinionTrait::FIGHTER})
        .setFrequency(0.6)
        .addRequirement(0.0, MinTurnRequirement{500_global})
        .addRequirement(0.1, AttractionInfo{1, {FurnitureType::BOOKCASE_WOOD, FurnitureType::LABORATORY}})
        .addSpecialTrait(0.03, {AttrBonus { AttrType::SPELL_DAMAGE, 7 }, LastingEffect::INSANITY})
        .addSpecialTrait(0.1, ExtraTraining { ExperienceType::SPELL, 4 })
        .addSpecialTrait(0.05, SkillId::LABORATORY)
        .addOneOrMoreTraits(0.05, {LastingEffect::HATE_ELVES, LastingEffect::HATE_HUMANS})
        .addSpecialTrait(0.1, LastingEffect::MAGIC_RESISTANCE),
    ImmigrantInfo(CreatureId::OGRE, {MinionTrait::FIGHTER})
        .setFrequency(0.3)
        .addRequirement(0.0, MinTurnRequirement{2000_global})
        .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_IRON})
        .addSpecialTrait(0.03, {AttrBonus { AttrType::DAMAGE, 5 }, LastingEffect::INSANITY})
        .addSpecialTrait(0.05, {AttrBonus { AttrType::DEFENSE, 5 }, LastingEffect::SLOWED})
        .addSpecialTrait(0.1, LastingEffect::RANGED_VULNERABILITY)
        .addOneOrMoreTraits(0.05, {LastingEffect::HATE_ELVES, LastingEffect::HATE_HUMANS})
        .addSpecialTrait(0.1, ExtraTraining { ExperienceType::ARCHERY, 2 }),
    ImmigrantInfo(CreatureId::HARPY, {MinionTrait::FIGHTER})
        .setFrequency(0.3)
        .addRequirement(0.0, MinTurnRequirement{2000_global})
        .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_WOOD})
        .addRequirement(0.3, AttractionInfo{1, ItemIndex::RANGED_WEAPON})
        .addSpecialTrait(0.03, {AttrBonus { AttrType::RANGED_DAMAGE, 5 }, LastingEffect::INSANITY})
        .addSpecialTrait(0.02, LastingEffect::INSANITY)
        .addSpecialTrait(0.2, LastingEffect::NIGHT_VISION)
        .addOneOrMoreTraits(0.05, {LastingEffect::HATE_ELVES, LastingEffect::HATE_HUMANS})
        .addSpecialTrait(0.1, ExtraTraining { ExperienceType::ARCHERY, 3 }),
    ImmigrantInfo(CreatureId::ZOMBIE, {MinionTrait::FIGHTER})
        .setFrequency(0.5)
        .addRequirement(0.0, MinTurnRequirement{1000_global})
        .setSpawnLocation(FurnitureType::GRAVE)
        .addRequirement(0.0, CostInfo(CollectiveResourceId::CORPSE, 1))
        .addSpecialTrait(0.3, LastingEffect::BLIND)
        .addSpecialTrait(0.3, LastingEffect::COLLAPSED),
    ImmigrantInfo(CreatureId::SKELETON, {MinionTrait::FIGHTER})
        .setFrequency(0.5)
        .addRequirement(0.0, MinTurnRequirement{1000_global})
        .setSpawnLocation(FurnitureType::GRAVE)
        .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_IRON})
        .addRequirement(0.0, CostInfo(CollectiveResourceId::CORPSE, 1)),
    ImmigrantInfo(CreatureId::VAMPIRE, {MinionTrait::FIGHTER})
        .setFrequency(0.2)
        .setSpawnLocation(FurnitureType::GRAVE)
        .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_IRON})
        .addRequirement(0.0, CostInfo(CollectiveResourceId::CORPSE, 1))
        .addSpecialTrait(0.3, LastingEffect::TELEPATHY)
        .addSpecialTrait(0.3, LastingEffect::FIRE_RESISTANT)
        .addSpecialTrait(0.05, LastingEffect::HATE_GREENSKINS)
        .addSpecialTrait(0.3, LastingEffect::FLYING),
    ImmigrantInfo(CreatureId::MUMMY, {MinionTrait::FIGHTER})
        .setFrequency(0.1)
        .addRequirement(0.0, MinTurnRequirement{2000_global})
        .setSpawnLocation(FurnitureType::GRAVE)
        .addRequirement(0.1, AttractionInfo{1, FurnitureType::TRAINING_IRON})
        .addRequirement(0.0, CostInfo(CollectiveResourceId::CORPSE, 1)),
    ImmigrantInfo(CreatureId::LOST_SOUL, {MinionTrait::FIGHTER})
        .setFrequency(0.3)
        .setSpawnLocation(FurnitureType::DEMON_SHRINE)
        .addRequirement(0.3, AttractionInfo{1, FurnitureType::DEMON_SHRINE})
        .addRequirement(0.0, FurnitureType::DEMON_SHRINE),
    ImmigrantInfo(CreatureId::SUCCUBUS, {MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT})
        .setFrequency(0.3)
        .setSpawnLocation(FurnitureType::DEMON_SHRINE)
        .addRequirement(0.3, AttractionInfo{2, FurnitureType::DEMON_SHRINE})
        .addRequirement(0.0, FurnitureType::DEMON_SHRINE),
    ImmigrantInfo(CreatureId::DOPPLEGANGER, {MinionTrait::FIGHTER})
        .setFrequency(0.3)
        .setSpawnLocation(FurnitureType::DEMON_SHRINE)
        .addRequirement(0.3, AttractionInfo{3, FurnitureType::DEMON_SHRINE})
        .addRequirement(0.0, FurnitureType::DEMON_SHRINE),
    ImmigrantInfo(CreatureId::RAVEN, {MinionTrait::FIGHTER, MinionTrait::DOESNT_TRIGGER})
        .setFrequency(0.5)
        .addRequirement(0.0, FurnitureType::BEAST_CAGE)
        .addRequirement(0.0, SunlightState::DAY),
    ImmigrantInfo(CreatureId::BAT, {MinionTrait::FIGHTER, MinionTrait::DOESNT_TRIGGER})
        .setFrequency(0.5)
        .addRequirement(0.0, FurnitureType::BEAST_CAGE)
        .addRequirement(0.0, SunlightState::NIGHT),
    ImmigrantInfo(CreatureId::WOLF, {MinionTrait::FIGHTER, MinionTrait::DOESNT_TRIGGER})
        .setFrequency(0.15)
        .addRequirement(0.0, FurnitureType::BEAST_CAGE)
        .setGroupSize(Range(3, 9))
        .setAutoTeam()
        .addRequirement(0.0, SunlightState::NIGHT),
    ImmigrantInfo(CreatureId::CAVE_BEAR, {MinionTrait::FIGHTER, MinionTrait::DOESNT_TRIGGER})
        .addRequirement(0.0, FurnitureType::BEAST_CAGE)
        .setFrequency(0.1),
    ImmigrantInfo(CreatureId::WEREWOLF, {MinionTrait::FIGHTER})
        .setFrequency(0.1)
        .addRequirement(0.0, MinTurnRequirement{2000_global})
        .addRequirement(0.1, AttractionInfo{2, FurnitureType::TRAINING_IRON})
        .addSpecialTrait(0.1, {AttrBonus { AttrType::DAMAGE, 5 }, LastingEffect::INSANITY})
        .addSpecialTrait(0.03, LastingEffect::INSANITY)
        .addSpecialTrait(0.1, LastingEffect::HATE_UNDEAD)
        .addSpecialTrait(0.3, SkillId::AMBUSH),
    ImmigrantInfo(CreatureId::DARK_ELF_WARRIOR, {MinionTrait::FIGHTER})
        .addRequirement(0.0, RecruitmentInfo{{EnemyId::DARK_ELVES}, 3, MinionTrait::FIGHTER})
        .addRequirement(CostInfo(CollectiveResourceId::GOLD, 20)),
    ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER})
        .addRequirement(0.0, RecruitmentInfo{{EnemyId::ORC_VILLAGE}, 3, MinionTrait::FIGHTER})
        .addRequirement(CostInfo(CollectiveResourceId::GOLD, 5)),
    ImmigrantInfo(CreatureId::HARPY, {MinionTrait::FIGHTER})
        .addRequirement(0.0, RecruitmentInfo{{EnemyId::HARPY_CAVE}, 3, MinionTrait::FIGHTER})
        .addRequirement(CostInfo(CollectiveResourceId::GOLD, 12)),
    ImmigrantInfo(CreatureId::OGRE, {MinionTrait::FIGHTER})
        .addRequirement(0.0, RecruitmentInfo{{EnemyId::OGRE_CAVE, EnemyId::ORC_VILLAGE}, 3, MinionTrait::FIGHTER})
        .addRequirement(CostInfo(CollectiveResourceId::GOLD, 12)),
    ImmigrantInfo(random.permutation({CreatureId::SPECIAL_HMBN, CreatureId::SPECIAL_HMBW,
            CreatureId::SPECIAL_HMGN, CreatureId::SPECIAL_HMGW}), {MinionTrait::FIGHTER})
        .setLimit(4)
        .addRequirement(0.0, TechId::HUMANOID_MUT)
        .addRequirement(0.0, Pregnancy {})
        .addRequirement(CostInfo(CollectiveResourceId::GOLD, 100))
        .setSpawnLocation(Pregnancy {})
        .addSpecialTrait(0.1, LastingEffect::INSANITY),
    ImmigrantInfo(random.permutation({CreatureId::SPECIAL_BMBN, CreatureId::SPECIAL_BMBW, CreatureId::SPECIAL_BMGN,
          CreatureId::SPECIAL_BMGW}), {MinionTrait::FIGHTER, MinionTrait::DOESNT_TRIGGER})
        .setLimit(4)
        .addRequirement(0.0, TechId::BEAST_MUT)
        .addRequirement(0.0, Pregnancy {})
        .addRequirement(CostInfo(CollectiveResourceId::GOLD, 100))
        .setSpawnLocation(Pregnancy {}),
  };
}

static vector<ImmigrantInfo> getWhiteKeeperImmigration(RandomGen& random) {
  return {
    ImmigrantInfo(CreatureId::KNIGHT_PLAYER, {MinionTrait::FIGHTER})
        .setFrequency(0.7)
        .addOneOrMoreTraits(0.1, {LastingEffect::HATE_ELVES, LastingEffect::HATE_DWARVES, LastingEffect::HATE_GREENSKINS})
        .addSpecialTrait(0.05, ExtraTraining { ExperienceType::ARCHERY, 2 })
        .addSpecialTrait(0.05, ExtraTraining { ExperienceType::MELEE, 2 })
        .addSpecialTrait(0.05, OneOfTraits{{LastingEffect::FAST_TRAINING, LastingEffect::SLOW_TRAINING}})
        .addRequirement(0.1, AttractionInfo{1, vector<AttractionType>({FurnitureType::TRAINING_WOOD})}),
    ImmigrantInfo(CreatureId::ARCHER_PLAYER, {MinionTrait::FIGHTER})
        .setFrequency(0.7)
        .addOneOrMoreTraits(0.1, {LastingEffect::HATE_ELVES, LastingEffect::HATE_DWARVES, LastingEffect::HATE_GREENSKINS})
        .addSpecialTrait(0.05, ExtraTraining { ExperienceType::ARCHERY, 2 })
        .addSpecialTrait(0.05, ExtraTraining { ExperienceType::MELEE, 2 })
        .addSpecialTrait(0.05, OneOfTraits{{LastingEffect::FAST_TRAINING, LastingEffect::SLOW_TRAINING}})
        .addRequirement(0.1, AttractionInfo{1, vector<AttractionType>({FurnitureType::ARCHERY_RANGE})}),
    ImmigrantInfo(CreatureId::PRIEST_PLAYER, {MinionTrait::FIGHTER})
        .setFrequency(0.3)
        .addSpecialTrait(0.2, LastingEffect::HATE_UNDEAD)
        .addSpecialTrait(0.05, ExtraTraining { ExperienceType::SPELL, 2 })
        .addSpecialTrait(0.03, {AttrBonus { AttrType::SPELL_DAMAGE, 4 }, AttrBonus { AttrType::DEFENSE, -4 }})
        .addRequirement(0.1, AttractionInfo{1, vector<AttractionType>({FurnitureType::BOOKCASE_WOOD})}),
    ImmigrantInfo(CreatureId::JESTER_PLAYER, {MinionTrait::FIGHTER})
        .setFrequency(0.1)
        .addRequirement(0.0, AttractionInfo{1, vector<AttractionType>({FurnitureType::THRONE})}),
    ImmigrantInfo(CreatureId::GNOME_PLAYER, {MinionTrait::FIGHTER, MinionTrait::NO_AUTO_EQUIPMENT})
        .setFrequency(0.7)
        .addSpecialTrait(0.05, OneOfTraits{{LastingEffect::FAST_CRAFTING, LastingEffect::SLOW_CRAFTING}})
/*        .addSpecialTrait(0.03, {SkillId::WORKSHOP, LastingEffect::INSANITY})
        .addSpecialTrait(0.03, {SkillId::FORGE, LastingEffect::INSANITY})
        .addSpecialTrait(0.03, {SkillId::JEWELER, LastingEffect::INSANITY}),*/
        .addRequirement(0.1, AttractionInfo{1, vector<AttractionType>(
             {FurnitureType::FORGE, FurnitureType::WORKSHOP, FurnitureType::JEWELER})}),
    ImmigrantInfo(CreatureId::DOG, {MinionTrait::FIGHTER, MinionTrait::DOESNT_TRIGGER, MinionTrait::NO_LIMIT})
        .setFrequency(0.1),
    ImmigrantInfo({CreatureId::DONKEY, CreatureId::COW, CreatureId::HORSE, CreatureId::GOAT},
            {MinionTrait::NO_LIMIT, MinionTrait::INCREASE_POPULATION})
        .addRequirement(CostInfo(CollectiveResourceId::GOLD, 50)),
  };
}

static CollectiveConfig getKeeperConfig(RandomGen& random, bool fastImmigration, bool regenerateMana,
    AvatarVariant avatarVariant) {
  vector<ImmigrantInfo> immigrants;
  switch (avatarVariant) {
    case AvatarVariant::DARK_MAGE:
      immigrants.push_back(
          ImmigrantInfo(CreatureId::IMP, {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT})
             .setSpawnLocation(NearLeader{})
             .setKeybinding(Keybinding::CREATE_IMP)
             .setSound(Sound(SoundId::CREATE_IMP).setPitch(2))
             .setNoAuto()
             .setInitialRecruitment(4)
             .addRequirement(ExponentialCost{ CostInfo(CollectiveResourceId::GOLD, 30), 5, 4 }));
      break;
    case AvatarVariant::DARK_KNIGHT:
      immigrants.push_back(
          ImmigrantInfo(CreatureId::PESEANT_PRISONER,
              {MinionTrait::WORKER, MinionTrait::PRISONER, MinionTrait::NO_LIMIT})
             .setSpawnLocation(NearLeader{})
             .setNoAuto()
             .setInvisible()
             .setInitialRecruitment(4));
      break;
    case AvatarVariant::WHITE_KNIGHT:
      immigrants.push_back(
          ImmigrantInfo(CreatureId::PESEANT_PLAYER, {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT})
             .setKeybinding(Keybinding::CREATE_IMP)
             .setNoAuto()
             .setInitialRecruitment(4)
             .addRequirement(ExponentialCost{ CostInfo(CollectiveResourceId::GOLD, 30), 5, 4 }));
      break;
  }
  switch (avatarVariant) {
    case AvatarVariant::DARK_MAGE:
    case AvatarVariant::DARK_KNIGHT:
      append(immigrants, getDarkKeeperImmigration(random));
      break;
    case AvatarVariant::WHITE_KNIGHT:
      append(immigrants, getWhiteKeeperImmigration(random));
      break;
  }
  return CollectiveConfig::keeper(
      TimeInterval(fastImmigration ? 10 : 140),
      10,
      regenerateMana,
      std::move(immigrants));
}

SettlementInfo& ModelBuilder::makeExtraLevel(WModel model, EnemyInfo& enemy) {
  const int towerHeight = random.get(7, 12);
  const int gnomeHeight = random.get(3, 5);
  SettlementInfo& mainSettlement = enemy.settlement;
  SettlementInfo& extraSettlement = enemy.levelConnection->otherEnemy->settlement;
  switch (enemy.levelConnection->type) {
    case LevelConnection::TOWER: {
      StairKey downLink = StairKey::getNew();
      extraSettlement.upStairs = {downLink};
      for (int i : Range(towerHeight - 1)) {
        StairKey upLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, 4, 4, "Tower floor" + toString(i + 2)),
            LevelMaker::towerLevel(random,
                CONSTRUCT(SettlementInfo,
                  c.type = SettlementType::TOWER;
                  c.inhabitants.fighters = CreatureList(
                      random.get(1, 3),
                      random.choose(
                          CreatureId::WATER_ELEMENTAL, CreatureId::AIR_ELEMENTAL, CreatureId::FIRE_ELEMENTAL,
                          CreatureId::EARTH_ELEMENTAL));
                  c.tribe = enemy.settlement.tribe;
                  c.collective = new CollectiveBuilder(CollectiveConfig::noImmigrants(), c.tribe);
                  c.upStairs = {upLink};
                  c.downStairs = {downLink};
                  c.furniture = FurnitureFactory(TribeId::getHuman(), FurnitureType::GROUND_TORCH);
                  if (enemy.levelConnection->deadInhabitants) {
                    c.corpses = c.inhabitants;
                    c.inhabitants = InhabitantsInfo{};
                  }
                  c.buildingId = BuildingId::BRICK;)));
        downLink = upLink;
      }
      mainSettlement.downStairs = {downLink};
      model->buildLevel(
         LevelBuilder(meter, random, 5, 5, "Tower top"),
         LevelMaker::towerLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::CRYPT: {
      StairKey key = StairKey::getNew();
      extraSettlement.downStairs = {key};
      mainSettlement.upStairs = {key};
      model->buildLevel(
         LevelBuilder(meter, random, 40, 40, "Crypt"),
         LevelMaker::cryptLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::MAZE: {
      StairKey key = StairKey::getNew();
      extraSettlement.upStairs = {key};
      mainSettlement.downStairs = {key};
      model->buildLevel(
         LevelBuilder(meter, random, 40, 40, "Maze"),
         LevelMaker::mazeLevel(random, extraSettlement));
      return mainSettlement;
    }
    case LevelConnection::GNOMISH_MINES: {
      StairKey upLink = StairKey::getNew();
      extraSettlement.downStairs = {upLink};
      for (int i : Range(gnomeHeight - 1)) {
        StairKey downLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, 60, 40, "Mines lvl " + toString(i + 1)),
            LevelMaker::roomLevel(random, CreatureFactory::gnomishMines(
                mainSettlement.tribe, TribeId::getMonster(), 0),
                CreatureFactory::waterCreatures(TribeId::getMonster()),
                CreatureFactory::lavaCreatures(TribeId::getMonster()), {upLink}, {downLink},
                FurnitureFactory::roomFurniture(TribeId::getPest())));
        upLink = downLink;
      }
      mainSettlement.upStairs = {upLink};
      model->buildLevel(
         LevelBuilder(meter, random, 60, 40, "Mine Town"),
         LevelMaker::mineTownLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::SOKOBAN:
      StairKey key = StairKey::getNew();
      extraSettlement.upStairs = {key};
      mainSettlement.downStairs = {key};
      Table<char> sokoLevel = sokobanInput->getNext();
      model->buildLevel(
          LevelBuilder(meter, random, sokoLevel.getBounds().width(), sokoLevel.getBounds().height(), "Sokoban"),
          LevelMaker::sokobanFromFile(random, mainSettlement, sokoLevel));
      return extraSettlement;
  }
}

PModel ModelBuilder::singleMapModel(const string& worldName, TribeId keeperTribe) {
  return tryBuilding(10, [&] { return trySingleMapModel(worldName, keeperTribe);}, "single map");
}

PModel ModelBuilder::trySingleMapModel(const string& worldName, TribeId keeperTribe) {
  vector<EnemyInfo> enemies;
  for (int i : Range(random.get(3, 6)))
    enemies.push_back(enemyFactory->get(EnemyId::HUMAN_COTTAGE));
  enemies.push_back(enemyFactory->get(EnemyId::KOBOLD_CAVE));
  for (int i : Range(random.get(2, 4)))
    enemies.push_back(enemyFactory->get(random.choose({EnemyId::BANDITS, EnemyId::COTTAGE_BANDITS}, {3, 1}))
        .setVillainType(VillainType::LESSER));
  enemies.push_back(enemyFactory->get(random.choose(EnemyId::GNOMES, EnemyId::DARK_ELVES)).setVillainType(VillainType::ALLY));
  append(enemies, enemyFactory->getVaults());
  enemies.push_back(enemyFactory->get(EnemyId::ANTS_CLOSED).setVillainType(VillainType::LESSER));
  enemies.push_back(enemyFactory->get(EnemyId::DWARVES).setVillainType(VillainType::MAIN));
  enemies.push_back(enemyFactory->get(EnemyId::KNIGHTS).setVillainType(VillainType::MAIN));
  enemies.push_back(enemyFactory->get(EnemyId::ADA_GOLEMS));
  enemies.push_back(enemyFactory->get(random.choose(EnemyId::OGRE_CAVE, EnemyId::HARPY_CAVE))
      .setVillainType(VillainType::ALLY));
  for (auto& enemy : random.chooseN(2, {
        EnemyId::ELEMENTALIST,
        EnemyId::WARRIORS,
        EnemyId::ELVES,
        EnemyId::VILLAGE}))
    enemies.push_back(enemyFactory->get(enemy).setVillainType(VillainType::MAIN));
  for (auto& enemy : random.chooseN(2, {
        EnemyId::GREEN_DRAGON,
        EnemyId::SHELOB,
        EnemyId::HYDRA,
        EnemyId::RED_DRAGON,
        EnemyId::CYCLOPS,
        EnemyId::DRIADS,
        EnemyId::ENTS}))
    enemies.push_back(enemyFactory->get(enemy).setVillainType(VillainType::LESSER));
  for (auto& enemy : random.chooseN(1, {
        EnemyId::KRAKEN,
        EnemyId::WITCH,
        EnemyId::CEMETERY}))
    enemies.push_back(enemyFactory->get(enemy));
  return tryModel(304, worldName, enemies, keeperTribe, BiomeId::GRASSLAND, {}, true);
}

void ModelBuilder::addMapVillains(vector<EnemyInfo>& enemyInfo, BiomeId biomeId) {
  switch (biomeId) {
    case BiomeId::GRASSLAND:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId::HUMAN_COTTAGE));
      if (random.roll(2))
        enemyInfo.push_back(enemyFactory->get(EnemyId::COTTAGE_BANDITS));
      break;
    case BiomeId::MOUNTAIN:
      for (int i : Range(random.get(1, 4)))
        enemyInfo.push_back(enemyFactory->get(random.choose(EnemyId::DWARF_CAVE, EnemyId::KOBOLD_CAVE)));
      for (int i : Range(random.get(0, 3)))
        enemyInfo.push_back(enemyFactory->get(random.choose({EnemyId::BANDITS, EnemyId::NO_AGGRO_BANDITS}, {1, 4})));
      break;
    case BiomeId::FORREST:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId::ELVEN_COTTAGE));
      break;
  }
}

PModel ModelBuilder::tryCampaignBaseModel(const string& siteName, TribeId keeperTribe, bool addExternalEnemies) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId::MOUNTAIN;
  enemyInfo.push_back(enemyFactory->get(EnemyId::DWARF_CAVE));
  enemyInfo.push_back(enemyFactory->get(EnemyId::BANDITS));
  enemyInfo.push_back(enemyFactory->get(EnemyId::ANTS_CLOSED_SMALL));
  enemyInfo.push_back(enemyFactory->get(EnemyId::ADA_GOLEMS));
  enemyInfo.push_back(enemyFactory->get(EnemyId::TUTORIAL_VILLAGE));
  append(enemyInfo, enemyFactory->getVaults());
  if (random.chance(0.3))
    enemyInfo.push_back(enemyFactory->get(EnemyId::KRAKEN));
  optional<ExternalEnemies> externalEnemies;
  if (addExternalEnemies)
    externalEnemies = ExternalEnemies(random, enemyFactory->getExternalEnemies());
  return tryModel(174, siteName, enemyInfo, keeperTribe, biome, std::move(externalEnemies), true);
}

PModel ModelBuilder::tryTutorialModel(const string& siteName) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId::MOUNTAIN;
  /*enemyInfo.push_back(enemyFactory->get(EnemyId::BANDITS));
  enemyInfo.push_back(enemyFactory->get(EnemyId::ADA_GOLEMS));*/
  enemyInfo.push_back(enemyFactory->get(EnemyId::KRAKEN));
  enemyInfo.push_back(enemyFactory->get(EnemyId::TUTORIAL_VILLAGE));
  return tryModel(174, siteName, enemyInfo, TribeId::getDarkKeeper(), biome, {}, false);
}

static optional<BiomeId> getBiome(EnemyInfo& enemy, RandomGen& random) {
  switch (enemy.settlement.type) {
    case SettlementType::CASTLE:
    case SettlementType::CASTLE2:
    case SettlementType::TOWER:
    case SettlementType::VILLAGE:
    case SettlementType::SWAMP:
      return BiomeId::GRASSLAND;
    case SettlementType::CAVE:
    case SettlementType::MINETOWN:
    case SettlementType::SMALL_MINETOWN:
    case SettlementType::ANT_NEST:
      return BiomeId::MOUNTAIN;
    case SettlementType::FORREST_COTTAGE:
    case SettlementType::FORREST_VILLAGE:
    case SettlementType::ISLAND_VAULT_DOOR:
    case SettlementType::FOREST:
      return BiomeId::FORREST;
    case SettlementType::CEMETERY:
      return random.choose(BiomeId::GRASSLAND, BiomeId::FORREST);
    default: return none;
  }
}

PModel ModelBuilder::tryCampaignSiteModel(const string& siteName, EnemyId enemyId, VillainType type) {
  vector<EnemyInfo> enemyInfo { enemyFactory->get(enemyId).setVillainType(type)};
  auto biomeId = getBiome(enemyInfo[0], random);
  CHECK(biomeId) << "Unimplemented enemy in campaign " << EnumInfo<EnemyId>::getString(enemyId);
  addMapVillains(enemyInfo, *biomeId);
  return tryModel(114, siteName, enemyInfo, none, *biomeId, {}, true);
}

PModel ModelBuilder::tryBuilding(int numTries, function<PModel()> buildFun, const string& name) {
  for (int i : Range(numTries)) {
    try {
      if (meter)
        meter->reset();
      return buildFun();
    } catch (LevelGenException) {
      INFO << "Retrying level gen";
    }
  }
  FATAL << "Couldn't generate a level: " << name;
  return nullptr;

}

PModel ModelBuilder::campaignBaseModel(const string& siteName, TribeId keeperTribe, bool externalEnemies) {
  return tryBuilding(20, [=] { return tryCampaignBaseModel(siteName, keeperTribe, externalEnemies); }, "campaign base");
}

PModel ModelBuilder::tutorialModel(const string& siteName) {
  return tryBuilding(20, [=] { return tryTutorialModel(siteName); }, "tutorial");
}

PModel ModelBuilder::campaignSiteModel(const string& siteName, EnemyId enemyId, VillainType type) {
  return tryBuilding(20, [&] { return tryCampaignSiteModel(siteName, enemyId, type); },
      EnumInfo<EnemyId>::getString(enemyId));
}

void ModelBuilder::measureSiteGen(int numTries, vector<string> types) {
  if (types.empty()) {
    types = {"single_map", "campaign_base", "tutorial"};
    for (auto id : ENUM_ALL(EnemyId)) {
      auto enemy = enemyFactory->get(id);
      if (!!getBiome(enemy, random))
        types.push_back(EnumInfo<EnemyId>::getString(id));
    }
  }
  vector<function<void()>> tasks;
  auto tribe = TribeId::getDarkKeeper();
  for (auto& type : types) {
    if (type == "single_map")
      tasks.push_back([=] { measureModelGen(type, numTries, [&] { trySingleMapModel("pok", tribe); }); });
    else if (type == "campaign_base")
      tasks.push_back([=] { measureModelGen(type, numTries, [&] { tryCampaignBaseModel("pok", tribe, false); }); });
    else if (type == "tutorial")
      tasks.push_back([=] { measureModelGen(type, numTries, [&] { tryTutorialModel("pok"); }); });
    else if (auto id = EnumInfo<EnemyId>::fromStringSafe(type)) {
      tasks.push_back([=] { measureModelGen(type, numTries, [&] { tryCampaignSiteModel("", *id, VillainType::LESSER); }); });
    } else {
      std::cout << "Bad map type: " << type << std::endl;
      return;
    }
  }
  for (auto& t : tasks)
    t();
}

void ModelBuilder::measureModelGen(const string& name, int numTries, function<void()> genFun) {
  int numSuccess = 0;
  int maxT = 0;
  int minT = 1000000;
  double sumT = 0;
  std::cout << name;
  for (int i : Range(numTries)) {
#ifndef OSX // this triggers some compiler errors OSX, I don't need it there anyway.
    auto time = steady_clock::now();
#endif
    try {
      genFun();
      ++numSuccess;
      std::cout << ".";
      std::cout.flush();
    } catch (LevelGenException) {
      std::cout << "x";
      std::cout.flush();
    }
#ifndef OSX
    int millis = duration_cast<milliseconds>(steady_clock::now() - time).count();
    sumT += millis;
    maxT = max(maxT, millis);
    minT = min(minT, millis);
#endif
  }
  std::cout << std::endl << numSuccess << " / " << numTries << ". MinT: " <<
    minT << ". MaxT: " << maxT << ". AvgT: " << sumT / numTries << std::endl;
}

WCollective ModelBuilder::spawnKeeper(WModel m, AvatarInfo avatarInfo, bool regenerateMana, vector<string> introText) {
  WLevel level = m->getTopLevel();
  WCreature keeperRef = avatarInfo.playerCreature.get();
  CHECK(level->landCreature(StairKey::keeperSpawn(), keeperRef)) << "Couldn't place keeper on level.";
  m->addCreature(std::move(avatarInfo.playerCreature));
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(random, options->getBoolValue(OptionId::FAST_IMMIGRATION),
            regenerateMana, *avatarInfo.avatarVariant), keeperRef->getTribeId())
      .setLevel(level)
      .addCreature(keeperRef, {MinionTrait::LEADER})
      .build());
  WCollective playerCollective = m->collectives.back().get();
  playerCollective->setControl(PlayerControl::create(playerCollective, introText, *avatarInfo.avatarVariant));
  playerCollective->setVillainType(VillainType::PLAYER);
  for (auto tech : Technology::getInitialTech(*avatarInfo.avatarVariant))
    playerCollective->acquireTech(tech, false);
  return playerCollective;
}

PModel ModelBuilder::tryModel(int width, const string& levelName, vector<EnemyInfo> enemyInfo,
    optional<TribeId> keeperTribe, BiomeId biomeId, optional<ExternalEnemies> externalEnemies, bool hasWildlife) {
  auto model = Model::create();
  vector<SettlementInfo> topLevelSettlements;
  vector<EnemyInfo> extraEnemies;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe);
    if (elem.levelConnection) {
      elem.levelConnection->otherEnemy->settlement.collective =
          new CollectiveBuilder(elem.levelConnection->otherEnemy->config,
                                elem.levelConnection->otherEnemy->settlement.tribe);
      topLevelSettlements.push_back(makeExtraLevel(model.get(), elem));
      extraEnemies.push_back(*elem.levelConnection->otherEnemy);
    } else
      topLevelSettlements.push_back(elem.settlement);
  }
  append(enemyInfo, extraEnemies);
  optional<CreatureFactory> wildlife;
  if (hasWildlife)
    wildlife = CreatureFactory::forrest(TribeId::getWildlife());
  WLevel top =  model->buildTopLevel(
      LevelBuilder(meter, random, width, width, levelName, false),
      LevelMaker::topLevel(random, wildlife, topLevelSettlements, width,
        keeperTribe, biomeId));
  model->calculateStairNavigation();
  for (auto& enemy : enemyInfo) {
    if (enemy.settlement.locationName)
      enemy.settlement.collective->setLocationName(*enemy.settlement.locationName);
    if (auto race = enemy.settlement.race)
      enemy.settlement.collective->setRaceName(*race);
    if (enemy.discoverable)
      enemy.settlement.collective->setDiscoverable();
    PCollective collective = enemy.settlement.collective->build();
    auto control = VillageControl::create(collective.get(), enemy.villain);
    if (enemy.villainType)
      collective->setVillainType(*enemy.villainType);
    if (enemy.id)
      collective->setEnemyId(*enemy.id);
    collective->setControl(std::move(control));
    model->collectives.push_back(std::move(collective));
  }
  if (externalEnemies)
    model->addExternalEnemies(std::move(*externalEnemies));
  return model;
}

PModel ModelBuilder::splashModel(const FilePath& splashPath) {
  auto m = Model::create();
  WLevel l = m->buildTopLevel(
      LevelBuilder(meter, Random, Level::getSplashBounds().width(), Level::getSplashBounds().height(), "Splash",
        true, 1.0),
      LevelMaker::splashLevel(
          CreatureFactory::splashLeader(TribeId::getHuman()),
          CreatureFactory::splashHeroes(TribeId::getHuman()),
          CreatureFactory::splashMonsters(TribeId::getDarkKeeper()),
          CreatureFactory::singleType(TribeId::getDarkKeeper(), CreatureId::IMP), splashPath));
  m->topLevel = l;
  return m;
}

PModel ModelBuilder::battleModel(const FilePath& levelPath, CreatureList allies, CreatureList enemies) {
  auto m = Model::create();
  ifstream stream(levelPath.getPath());
  Table<char> level = *SokobanInput::readTable(stream);
  WLevel l = m->buildTopLevel(
      LevelBuilder(meter, Random, level.getBounds().width(), level.getBounds().height(), "Battle", true, 1.0),
      LevelMaker::battleLevel(level, allies, enemies));
  m->topLevel = l;
  return m;
}
