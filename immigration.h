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

  struct Group {
    ImmigrantInfo info;
    int count;
  };

  class Available {
    public:
    static Available generate(Collective*, const Group& group);
    vector<Position> getSpawnPositions(const Collective*) const;
    vector<Creature*> getCreatures() const;
    double getEndTime() const;

    SERIALIZATION_DECL(Available)

    private:
    Available(vector<PCreature>, ImmigrantInfo, double endTime);
    friend class Immigration;
    vector<PCreature> SERIAL(creatures);
    ImmigrantInfo SERIAL(info);
    double SERIAL(endTime);
  };

  map<int, std::reference_wrapper<const Available>> getAvailable(const Collective*) const;

  bool isUnavailable(const Available&, const Collective*) const;
  void accept(Collective*, int id);
  void reject(int id);
  vector<string> getMissingRequirements(const Collective* collective, const Available&) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  int getAttractionOccupation(const Collective* collective, const AttractionType& attraction) const;
  int getAttractionValue(const Collective* collective, const AttractionType& attraction) const;
  static vector<string> getAllRequirements(const ImmigrantInfo&);

  private:
  EntityMap<Creature, map<AttractionType, int>> SERIAL(minionAttraction);
  map<int, Available> SERIAL(available);
  double getImmigrantChance(const Collective* collective, const Group& info) const;
  vector<string> getMissingAttractions(const Collective*, const ImmigrantInfo&) const;
  int SERIAL(idCnt) = 0;
  void occupyAttraction(const Collective*, const Creature*, const AttractionInfo&);
  void occupyRequirements(Collective*, const Creature*, const ImmigrantInfo&);
  bool preliminaryRequirementsMet(const Collective*, const Group&) const;
  vector<string> getMissingRequirements(const Collective*, const Group&) const;
};
