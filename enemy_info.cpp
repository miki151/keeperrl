#include "enemy_info.h"
#include "immigrant_info.h"

SERIALIZE_DEF(EnemyInfo, NAMED(settlement), OPTION(config), NAMED(behaviour), NAMED(levelConnection), OPTION(immigrants), OPTION(discoverable), NAMED(createOnBones), OPTION(biomes), NAMED(otherEnemy))

SERIALIZATION_CONSTRUCTOR_IMPL(EnemyInfo)

EnemyInfo::EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v,
    optional<LevelConnection> l)
  : settlement(s), config(c), behaviour(v), levelConnection(l) {
}

STRUCT_IMPL(EnemyInfo)

EnemyInfo& EnemyInfo::setVillainType(VillainType type) {
  villainType = type;
  return *this;
}

EnemyInfo& EnemyInfo::setId(EnemyId i) {
  id = i;
  return *this;
}

EnemyInfo& EnemyInfo::setImmigrants(vector<ImmigrantInfo> i) {
  immigrants = std::move(i);
  return *this;
}

EnemyInfo& EnemyInfo::setNonDiscoverable() {
  discoverable = false;
  return *this;
}

template <class Archive>
void LevelConnection::LevelInfo::serialize(Archive& ar, unsigned int v) {
  ar(NAMED(enemy), NAMED(levelSize), NAMED(levelType), NAMED(name), OPTION(isLit), OPTION(canTransfer));
}

#include "pretty_archive.h"
template
void EnemyInfo::serialize(PrettyInputArchive& ar1, unsigned);

template
void LevelConnection::LevelInfo::serialize(PrettyInputArchive& ar, unsigned int v);
