#include "keeper_creature_info.h"
#include "special_trait.h"
#include "creature_id.h"
#include "tribe_alignment.h"
#include "lasting_effect.h"
#include "skill.h"
#include "attr_type.h"

SERIALIZE_DEF(KeeperCreatureInfo, NAMED(creatureId), NAMED(tribeAlignment), NAMED(immigrantGroups), NAMED(technology), NAMED(initialTech), NAMED(buildingGroups), NAMED(workshopGroups), NAMED(description), OPTION(specialTraits), OPTION(noLeader), NAMED(baseNameGen), OPTION(minionTraits), OPTION(maxPopulation), OPTION(immigrantInterval), OPTION(populationString), OPTION(prisoners), OPTION(endlessEnemyGroups), NAMED(unlock), OPTION(credit), OPTION(flags));

KeeperCreatureInfo::KeeperCreatureInfo() {
}

STRUCT_IMPL(KeeperCreatureInfo)

#include "pretty_archive.h"
template
void KeeperCreatureInfo::serialize(PrettyInputArchive& ar1, unsigned);
