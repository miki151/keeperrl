#pragma once

#include "stdafx.h"
#include "util.h"
#include "entity_map.h"
#include "entity_set.h"
#include "collective_config.h"
#include "immigrant_info.h"
#include "immigrant_auto_state.h"
#include "game_time.h"

class Collective;
struct AttractionInfo;

class Immigration : public OwnedObject<Immigration> {
  public:
  Immigration(WCollective);
  void update();

  struct Group {
    int immigrantIndex;
    int count;
  };

  class Available {
    public:
    vector<WCreature> getCreatures() const;
    optional<GlobalTime> getEndTime() const;
    optional<CostInfo> getCost() const;
    const ImmigrantInfo& getInfo() const;
    bool isUnavailable() const;
    optional<milliseconds> getCreatedTime() const;
    int getImmigrantIndex() const;
    const vector<SpecialTrait>& getSpecialTraits() const;

    SERIALIZATION_DECL(Available)

    private:
    static Available generate(WImmigration, const Group& group);
    static Available generate(WImmigration, int index);
    vector<Position> getSpawnPositions() const;
    Available(WImmigration, vector<PCreature>, int immigrantIndex, optional<GlobalTime> endTime, vector<SpecialTrait>);
    void addAllCreatures(const vector<Position>& spawnPositions);
    friend class Immigration;
    vector<PCreature> SERIAL(creatures);
    int SERIAL(immigrantIndex);
    optional<GlobalTime> SERIAL(endTime);
    WImmigration SERIAL(immigration);
    vector<SpecialTrait> SERIAL(specialTraits);
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
  EntityMap<Creature, unordered_map<AttractionType, int, CustomHash<AttractionType>>> SERIAL(minionAttraction);
  map<int, Available> SERIAL(available);
  WCollective SERIAL(collective);
  map<int, EntitySet<Creature>> SERIAL(generated);
  double getImmigrantChance(const Group& info) const;
  vector<string> getMissingAttractions(const ImmigrantInfo&) const;
  int SERIAL(idCnt) = 0;
  TimeInterval SERIAL(candidateTimeout);
  void occupyAttraction(WConstCreature, const AttractionInfo&);
  void occupyRequirements(WConstCreature, int immigrantIndex);
  double getRequirementMultiplier(const Group&) const;
  vector<string> getMissingRequirements(const Group&) const;
  void considerPersistentImmigrants(const vector<ImmigrantInfo>&);
  CostInfo calculateCost(int index, const ExponentialCost&) const;
  bool SERIAL(initialized) = false;
  void initializePersistent();
  optional<GlobalTime> SERIAL(nextImmigrantTime);
  void resetImmigrantTime();
  map<int, ImmigrantAutoState> SERIAL(autoState);
  int getNumGeneratedAndCandidates(int index) const;
};
