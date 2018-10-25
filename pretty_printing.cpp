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
#include "trap_type.h"
#include "campaign.h"
#include "enemy_factory.h"
#include "keeper_creature_info.h"
#include "adventurer_creature_info.h"
#include "build_info.h"
#include "furniture_layer.h"
#include "tutorial_highlight.h"
#include "workshop_item.h"
#include "workshop_type.h"

template <typename T>
optional<string> PrettyPrinting::parseObject(T& object, const string& s) {
  try {
    PrettyInput input(s);
    input(object);
    return none;
  } catch (PrettyException ex) {
    return ex.text;
  }
}

template
optional<string> PrettyPrinting::parseObject<Effect>(Effect&, const string&);

template
optional<string> PrettyPrinting::parseObject<ItemType>(ItemType&, const string&);

template
optional<string> PrettyPrinting::parseObject<CreatureId>(CreatureId&, const string&);


using VillainsTuple = tuple<vector<Campaign::VillainInfo>, vector<Campaign::VillainInfo>,
    vector<Campaign::VillainInfo>, vector<Campaign::VillainInfo>>;

template
optional<string> PrettyPrinting::parseObject<VillainsTuple>(VillainsTuple&, const string&);

using PlayerCreatureData = pair<vector<KeeperCreatureInfo>, vector<AdventurerCreatureInfo>>;

template
optional<string> PrettyPrinting::parseObject<PlayerCreatureData>(PlayerCreatureData&, const string&);

using BuildData = pair<vector<BuildInfo>, vector<BuildInfo>>;

template
optional<string> PrettyPrinting::parseObject<BuildData>(BuildData&, const string&);

using WorkshopData = std::array<vector<WorkshopItemCfg>, EnumInfo<WorkshopType>::size>;

template
optional<string> PrettyPrinting::parseObject<WorkshopData>(WorkshopData&, const string&);
