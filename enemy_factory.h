#pragma once

#include "util.h"
#include "villain_type.h"
#include "enemy_id.h"

struct EnemyInfo;
struct BuildingInfo;

struct ExternalEnemy;
struct SettlementInfo;
class TribeId;
class NameGenerator;

class EnemyFactory {
  public:
  EnemyFactory(RandomGen&, NameGenerator*, map<EnemyId, EnemyInfo> enemies, map<BuildingId, BuildingInfo> buildingInfo,
      vector<ExternalEnemy>);
  EnemyFactory(const EnemyFactory&) = delete;
  EnemyFactory(EnemyFactory&&) = default;
  EnemyInfo get(EnemyId) const;
  vector<ExternalEnemy> getExternalEnemies() const;
  vector<EnemyId> getAllIds() const;

  private:
  RandomGen& random;
  NameGenerator* nameGenerator;
  map<EnemyId, EnemyInfo> enemies;
  map<BuildingId, BuildingInfo> buildingInfo;
  vector<ExternalEnemy> externalEnemies;
  void updateCreateOnBones(EnemyInfo&) const;
};
