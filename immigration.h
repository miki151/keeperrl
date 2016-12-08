#pragma once

#include "stdafx.h"
#include "util.h"
#include "entity_map.h"
#include "collective_config.h"

class Collective;
struct ImmigrantInfo;
struct AttractionInfo;

class Immigration {
  public:

  void update(Collective*);

  class Available {
    public:
    static Available generate(Collective*, const ImmigrantInfo&);
    bool isUnavailable(const Collective*) const;
    vector<Position> getSpawnPositions(const Collective*) const;
    CostInfo getCost() const;

    SERIALIZATION_DECL(Available)

    private:
    Available(vector<PCreature>, ImmigrantInfo, double endTime);
    friend class Immigration;
    vector<PCreature> SERIAL(creatures);
    ImmigrantInfo SERIAL(info);
    double SERIAL(endTime);
  };

  map<int, std::reference_wrapper<const Available>> getAvailable(const Collective*) const;

  void accept(Collective*, int id);
  void reject(int id);
  bool canAccept(const Collective* collective, Available&) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EntityMap<Creature, vector<AttractionInfo>> SERIAL(minionAttraction);
  map<int, Available> SERIAL(available);
  double getAttractionOccupation(const Collective* collective, const MinionAttraction& attraction) const;
  double getAttractionValue(const Collective* collective, const MinionAttraction& attraction) const;
  optional<double> getImmigrantChance(const Collective* collective, const ImmigrantInfo& info) const;
  int SERIAL(idCnt) = 0;
};
