#pragma once

#include "util.h"
#include "villain_type.h"
#include "enemy_id.h"

struct EnemyInfo;

struct ExternalEnemy;
struct SettlementInfo;
class TribeId;
class NameGenerator;

class EnemyFactory {
  public:
  EnemyFactory(RandomGen&, NameGenerator*, map<EnemyId, EnemyInfo> enemies);
  EnemyFactory(const EnemyFactory&) = delete;
  EnemyFactory(EnemyFactory&&) = default;
  EnemyInfo get(EnemyId) const;
  vector<ExternalEnemy> getExternalEnemies() const;
  vector<ExternalEnemy> getHalloweenKids();
  vector<EnemyInfo> getVaults(TribeAlignment, TribeId allied) const;
  vector<EnemyId> getAllIds() const;

  private:
  RandomGen& random;
  NameGenerator* nameGenerator;
  map<EnemyId, EnemyInfo> enemies;
  void updateCreateOnBones(EnemyInfo&) const;
};
