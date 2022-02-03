#include "stdafx.h"
#include "pretty_printing.h"
#include "pretty_archive.h"
#include "creature_factory.h"
#include "furniture_type.h"
#include "attack_type.h"
#include "effect.h"
#include "lasting_effect.h"
#include "attr_type.h"
#include "body.h"
#include "item_type.h"
#include "technology.h"
#include "campaign.h"
#include "enemy_factory.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"
#include "build_info.h"
#include "furniture_layer.h"
#include "tutorial_highlight.h"
#include "workshop_item.h"
#include "workshop_type.h"
#include "immigrant_info.h"
#include "technology.h"
#include "tribe_alignment.h"
#include "skill.h"
#include "creature_attributes.h"
#include "spell_map.h"
#include "view_object.h"
#include "z_level_info.h"
#include "resource_counts.h"
#include "creature_inventory.h"
#include "campaign_type.h"
#include "tile_info.h"
#include "furniture.h"
#include "enemy_info.h"
#include "name_generator.h"
#include "furniture_list.h"
#include "external_enemies.h"
#include "test_struct.h"
#include "item_attributes.h"
#include "biome_id.h"
#include "biome_info.h"
#include "music_type.h"
#include "building_info.h"
#include "name_generator_id.h"
#include "campaign_builder.h"
#include "content_factory.h"
#include "storage_id.h"
#include "layout_generator.h"
#include "scripted_ui.h"
#include "storage_id.h"
#include "tile_gas_type.h"
#include "tile_gas_info.h"
#include "promotion_info.h"
#include "dancing.h"

template <typename T>
optional<string> PrettyPrinting::parseObject(T& object, const vector<string>& s, vector<string> filename, KeyVerifier* keyVerifier) {
  try {
    PrettyInputArchive input(s, filename, keyVerifier);
    input(object);
    return none;
  } catch (PrettyException ex) {
    return ex.text;
  }
}

#define ADD_IMP(...) \
template \
optional<string> PrettyPrinting::parseObject<__VA_ARGS__>(__VA_ARGS__&, const vector<string>&, vector<string>, KeyVerifier*);

ADD_IMP(Effect)
ADD_IMP(ItemType)
ADD_IMP(CreatureId)
ADD_IMP(VillainsTuple)
ADD_IMP(map<string, KeeperCreatureInfo>)
ADD_IMP(map<string, AdventurerCreatureInfo>)
ADD_IMP(map<string, vector<BuildInfo>>)
ADD_IMP(map<string, WorkshopArray>)
ADD_IMP(map<string, vector<ImmigrantInfo>>)
ADD_IMP(map<string, string>)
ADD_IMP(map<PrimaryId<CreatureId>, CreatureAttributes>)
ADD_IMP(map<PrimaryId<TechId>, Technology::TechDefinition>)
ADD_IMP(map<string, vector<ZLevelInfo>>)
ADD_IMP(vector<ResourceDistribution>)
ADD_IMP(GameIntros)
ADD_IMP(map<PrimaryId<SpellId>, Spell>)
ADD_IMP(map<PrimaryId<SpellSchoolId>, SpellSchool>)
ADD_IMP(vector<TileInfo>)
ADD_IMP(map<PrimaryId<FurnitureType>, Furniture>)
ADD_IMP(map<PrimaryId<EnemyId>, EnemyInfo>)
ADD_IMP(map<string, vector<ExternalEnemy>>)
ADD_IMP(map<PrimaryId<ItemListId>, ItemList>)
ADD_IMP(map<PrimaryId<FurnitureListId>, FurnitureList>)
ADD_IMP(map<PrimaryId<CustomItemId>, ItemAttributes>)
ADD_IMP(map<PrimaryId<BuildingId>, BuildingInfo>)
ADD_IMP(map<PrimaryId<BiomeId>, BiomeInfo>)
ADD_IMP(map<PrimaryId<WorkshopType>, WorkshopInfo>)
ADD_IMP(map<PrimaryId<LayoutMappingId>, LayoutMapping>)
ADD_IMP(map<PrimaryId<RandomLayoutId>, LayoutGenerator>)
ADD_IMP(map<string, CampaignInfo>)
ADD_IMP(map<PrimaryId<NameGeneratorId>, vector<string>>)
ADD_IMP(vector<pair<PrimaryId<CollectiveResourceId>, ResourceInfo>>)
ADD_IMP(map<string, TestStruct2>)
ADD_IMP(map<string, TestStruct3>)
ADD_IMP(map<string, TestStruct5>)
ADD_IMP(map<string, vector<int>>)
ADD_IMP(ScriptedUI)
ADD_IMP(vector<PrimaryId<StorageId>>)
ADD_IMP(map<PrimaryId<TileGasType>, TileGasInfo>)
ADD_IMP(map<string, vector<PromotionInfo>>)
ADD_IMP(vector<Dancing::Positions>)
ADD_IMP(vector<pair<string, ViewId>>)
ADD_IMP(vector<ScriptedHelpInfo>)
