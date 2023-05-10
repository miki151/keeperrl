#include "enemy_info.h"
#include "immigrant_info.h"

SERIALIZE_DEF(EnemyInfo, NAMED(settlement), OPTION(config), NAMED(behaviour), NAMED(levelConnection), OPTION(immigrants), OPTION(discoverable), NAMED(createOnBones), OPTION(biome), NAMED(otherEnemy))

SERIALIZATION_CONSTRUCTOR_IMPL(EnemyInfo)

EnemyInfo::EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v,
    optional<LevelConnection> l)
  : settlement(s), config(c), behaviour(v), levelConnection(l) {
}

void EnemyInfo::updateBuildingInfo(const map<BuildingId, BuildingInfo>& info) {
  using namespace MapLayoutTypes;
  settlement.type.visit(
        [&](const Builtin& elem) { const_cast<Builtin&>(elem).buildingInfo = info.at(elem.buildingId); },
        [&](const Predefined& elem) { const_cast<Predefined&>(elem).buildingInfo = info.at(elem.buildingId); },
        [&](const RandomLayout&) {}
  );
}

optional<BiomeId> EnemyInfo::getBiome() const {
  if (!!biome)
    return biome;
  return settlement.type.visit(
      [&](const MapLayoutTypes::Builtin& type) -> optional<BiomeId> {
        switch (type.id) {
          case BuiltinLayoutId::CASTLE2:
          case BuiltinLayoutId::TOWER:
          case BuiltinLayoutId::VILLAGE:
            return BiomeId("GRASSLAND");
          case BuiltinLayoutId::CAVE:
          case BuiltinLayoutId::MINETOWN:
          case BuiltinLayoutId::SMALL_MINETOWN:
          case BuiltinLayoutId::ANT_NEST:
          case BuiltinLayoutId::VAULT:
            return BiomeId("MOUNTAIN");
          case BuiltinLayoutId::FORREST_COTTAGE:
          case BuiltinLayoutId::FORREST_VILLAGE:
          case BuiltinLayoutId::ISLAND_VAULT_DOOR:
          case BuiltinLayoutId::FOREST:
            return BiomeId("FOREST");
          case BuiltinLayoutId::CEMETERY:
            return BiomeId("GRASSLAND");
          default: return none;
        }
      },
      [&](const auto&) -> optional<BiomeId> {
        return BiomeId("GRASSLAND");
      }
    );
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
  ar(NAMED(enemy), NAMED(levelSize), NAMED(levelType), NAMED(name), OPTION(isLit), OPTION(canTransfer), OPTION(aiFollows));
}

#include "pretty_archive.h"
template
void EnemyInfo::serialize(PrettyInputArchive& ar1, unsigned);

template
void LevelConnection::LevelInfo::serialize(PrettyInputArchive& ar, unsigned int v);
