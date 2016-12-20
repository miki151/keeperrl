#pragma once

#include "stdafx.h"
#include "util.h"
#include "entity_map.h"
#include "collective_config.h"
#include "immigrant_info.h"

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
    vector<Creature*> getCreatures() const;
    optional<double> getEndTime() const;
    optional<CostInfo> getCost() const;
    const ImmigrantInfo& getInfo() const;
    bool isUnavailable() const;

    SERIALIZATION_DECL(Available)

    private:
    static Available generate(Immigration*, const Group& group);
    vector<Position> getSpawnPositions() const;
    Available(Immigration*, vector<PCreature>, int immigrantIndex, optional<double> endTime);
    void regenerate();
    friend class Immigration;
    vector<PCreature> SERIAL(creatures);
    int SERIAL(immigrantIndex);
    optional<double> SERIAL(endTime);
    Immigration* SERIAL(immigration);
  };

  map<int, std::reference_wrapper<const Available>> getAvailable() const;

  void accept(int id);
  void reject(int id);
  vector<string> getMissingRequirements(const Available&) const;

  SERIALIZATION_DECL(Immigration)

  int getAttractionOccupation(const AttractionType& attraction) const;
  int getAttractionValue(const AttractionType& attraction) const;
  static vector<string> getAllRequirements(const ImmigrantInfo&);

  private:
  vector<ImmigrantInfo> SERIAL(immigrants);
  EntityMap<Creature, map<AttractionType, int>> SERIAL(minionAttraction);
  map<int, Available> SERIAL(available);
  Collective* SERIAL(collective);
  map<int, vector<Creature*>> SERIAL(generated);
  double getImmigrantChance(const Group& info) const;
  vector<string> getMissingAttractions(const ImmigrantInfo&) const;
  int SERIAL(idCnt) = 0;
  int SERIAL(candidateTimeout);
  void occupyAttraction(const Creature*, const AttractionInfo&);
  void occupyRequirements(const Creature*, int immigrantIndex);
  bool preliminaryRequirementsMet(const Group&) const;
  vector<string> getMissingRequirements(const Group&) const;
  void considerPersistentImmigrants(const vector<ImmigrantInfo>&);
  CostInfo calculateCost(int index, const ExponentialCost&) const;
};
