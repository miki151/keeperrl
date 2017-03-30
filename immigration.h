#pragma once

#include "stdafx.h"
#include "util.h"
#include "entity_map.h"
#include "entity_set.h"
#include "collective_config.h"
#include "immigrant_info.h"
#include "immigrant_auto_state.h"

class Collective;
struct AttractionInfo;

class Immigration {
  public:
  Immigration(Collective*);
  void update();

  struct Group {
    int immigrantIndex;
    int count;
  };

  class Available {
    public:
    vector<WCreature> getCreatures() const;
    optional<double> getEndTime() const;
    optional<CostInfo> getCost() const;
    const ImmigrantInfo& getInfo() const;
    bool isUnavailable() const;
    optional<milliseconds> getCreatedTime() const;

    SERIALIZATION_DECL(Available)

    private:
    static Available generate(Immigration*, const Group& group);
    static Available generate(Immigration*, int index);
    vector<Position> getSpawnPositions() const;
    Available(Immigration*, vector<PCreature>, int immigrantIndex, optional<double> endTime);
    void addAllCreatures(const vector<Position>& spawnPositions);
    friend class Immigration;
    vector<PCreature> SERIAL(creatures);
    int SERIAL(immigrantIndex);
    optional<double> SERIAL(endTime);
    Immigration* SERIAL(immigration);
    optional<milliseconds> createdTime;
  };

  map<int, std::reference_wrapper<const Available>> getAvailable() const;

  void accept(int id, bool withMessage = true);
  void rejectIfNonPersistent(int id);
  vector<string> getMissingRequirements(const Available&) const;

  void setAutoState(int index, optional<ImmigrantAutoState>);
  optional<ImmigrantAutoState> getAutoState(int index) const;

  SERIALIZATION_DECL(Immigration)

  int getAttractionOccupation(const AttractionType& attraction) const;
  int getAttractionValue(const AttractionType& attraction) const;

  private:
  const vector<ImmigrantInfo>& getImmigrants() const;
  EntityMap<Creature, map<AttractionType, int>> SERIAL(minionAttraction);
  map<int, Available> SERIAL(available);
  Collective* SERIAL(collective);
  map<int, EntitySet<Creature>> SERIAL(generated);
  double getImmigrantChance(const Group& info) const;
  vector<string> getMissingAttractions(const ImmigrantInfo&) const;
  int SERIAL(idCnt) = 0;
  int SERIAL(candidateTimeout);
  void occupyAttraction(WConstCreature, const AttractionInfo&);
  void occupyRequirements(WConstCreature, int immigrantIndex);
  double getRequirementMultiplier(const Group&) const;
  vector<string> getMissingRequirements(const Group&) const;
  void considerPersistentImmigrants(const vector<ImmigrantInfo>&);
  CostInfo calculateCost(int index, const ExponentialCost&) const;
  bool SERIAL(initialized) = false;
  void initializePersistent();
  double SERIAL(nextImmigrantTime) = -1;
  void resetImmigrantTime();
  map<int, ImmigrantAutoState> SERIAL(autoState);
};
