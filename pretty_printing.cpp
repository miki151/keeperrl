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

template <typename T>
optional<string> PrettyPrinting::parseObject(T& object, const string& s, optional<string> filename, KeyVerifier* keyVerifier) {
  try {
    PrettyInput input(s, filename, keyVerifier);
    input(object);
    return none;
  } catch (PrettyException ex) {
    return ex.text;
  }
}

#define ADD_IMP(...) \
template \
optional<string> PrettyPrinting::parseObject<__VA_ARGS__>(__VA_ARGS__&, const string&, optional<string>, KeyVerifier*);

ADD_IMP(Effect)
ADD_IMP(ItemType)
ADD_IMP(CreatureId)
ADD_IMP(array<vector<Campaign::VillainInfo>, 4>)
ADD_IMP(pair<vector<KeeperCreatureInfo>, vector<AdventurerCreatureInfo>>)
ADD_IMP(vector<pair<string, vector<BuildInfo>>>)
ADD_IMP(vector<pair<string, std::array<vector<WorkshopItemCfg>, EnumInfo<WorkshopType>::size>>>)
ADD_IMP(map<string, vector<ImmigrantInfo>>)
ADD_IMP(map<string, string>)
ADD_IMP(map<PrimaryId<CreatureId>, CreatureAttributes>)
ADD_IMP(vector<pair<vector<CreatureId>, CreatureInventory>>)
ADD_IMP(Technology)
ADD_IMP(array<vector<ZLevelInfo>, 3>)
ADD_IMP(vector<ResourceDistribution>)
ADD_IMP(pair<vector<string>, map<CampaignType, vector<string>>>)
ADD_IMP(Spell)
ADD_IMP(vector<Spell>)
ADD_IMP(map<PrimaryId<SpellSchoolId>, SpellSchool>)
ADD_IMP(vector<TileInfo>)
ADD_IMP(map<PrimaryId<FurnitureType>, Furniture>)
ADD_IMP(map<PrimaryId<EnemyId>, EnemyInfo>)
ADD_IMP(vector<ExternalEnemy>)
ADD_IMP(map<PrimaryId<ItemListId>, ItemList>)
ADD_IMP(map<PrimaryId<FurnitureListId>, FurnitureList>)
