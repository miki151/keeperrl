#include "stdafx.h"
#include "immigrant_info.h"
#include "furniture.h"
#include "item_index.h"
#include "game.h"
#include "collective.h"
#include "creature.h"
#include "creature_attributes.h"

SERIALIZE_DEF(ImmigrantInfo, ids, frequency, requirements, traits, spawnLocation, groupSize, autoTeam, initialRecruitment, consumeIds, keybinding, sound, noAuto, tutorialHighlight)
SERIALIZATION_CONSTRUCTOR_IMPL(ImmigrantInfo)

AttractionInfo::AttractionInfo(int cl,  AttractionType a)
  : types({a}), amountClaimed(cl) {}

string AttractionInfo::getAttractionName(const AttractionType& attraction, int count) {
  return apply_visitor(attraction, makeVisitor<string>(
      [&](FurnitureType type) {
        return Furniture::getName(type, count);
      },
      [&](ItemIndex index) {
        return getName(index, count);
      }
  ));
}

AttractionInfo::AttractionInfo(int cl, vector<AttractionType> a)
  : types(a), amountClaimed(cl) {}

ImmigrantInfo::ImmigrantInfo(CreatureId id, EnumSet<MinionTrait> t) : ids({id}), traits(t) {}
ImmigrantInfo::ImmigrantInfo(vector<CreatureId> id, EnumSet<MinionTrait> t) : ids(id), traits(t), consumeIds(true) {
}

CreatureId ImmigrantInfo::getId(int numCreated) const {
  if (!consumeIds)
    return Random.choose(ids);
  else
    return ids.at(numCreated);
}

bool ImmigrantInfo::isAvailable(int numCreated) const {
  if (!consumeIds)
    return true;
  else
    return numCreated < ids.size();
}

const SpawnLocation& ImmigrantInfo::getSpawnLocation() const {
  return spawnLocation;
}

Range ImmigrantInfo::getGroupSize() const {
  return groupSize;
}

int ImmigrantInfo::getInitialRecruitment() const {
  return initialRecruitment;
}

bool ImmigrantInfo::isAutoTeam() const {
  return autoTeam;
}

double ImmigrantInfo::getFrequency() const {
  CHECK(frequency);
  return *frequency;
}

bool ImmigrantInfo::isPersistent() const {
  return !frequency;
}

const EnumSet<MinionTrait>&ImmigrantInfo::getTraits() const {
  return traits;
}

optional<int> ImmigrantInfo::getLimit() const {
  if (consumeIds)
    return ids.size();
  else
    return none;
}

optional<Keybinding> ImmigrantInfo::getKeybinding() const {
  return keybinding;
}

optional<Sound> ImmigrantInfo::getSound() const {
  return sound;
}

bool ImmigrantInfo::isNoAuto() const {
  return noAuto;
}

optional<TutorialHighlight> ImmigrantInfo::getTutorialHighlight() const {
  return tutorialHighlight;
}

ImmigrantInfo& ImmigrantInfo::addRequirement(ImmigrantRequirement t) {
  requirements.push_back({t, 1});
  return *this;
}

ImmigrantInfo& ImmigrantInfo::addRequirement(double prob, ImmigrantRequirement t) {
  requirements.push_back({t, prob});
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setFrequency(double f) {
  frequency = f;
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setSpawnLocation(SpawnLocation l) {
  spawnLocation = l;
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setInitialRecruitment(int i) {
  initialRecruitment = i;
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setGroupSize(Range r) {
  groupSize = r;
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setAutoTeam() {
  autoTeam = true;
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setKeybinding(Keybinding key) {
  keybinding = key;
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setSound(Sound s) {
  sound = s;
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setNoAuto() {
  noAuto = true;
  return *this;
}

ImmigrantInfo&ImmigrantInfo::setLimit(int num) {
  consumeIds = true;
  ids = vector<CreatureId>(num, ids[0]);
  return *this;
}

ImmigrantInfo& ImmigrantInfo::setTutorialHighlight(TutorialHighlight h) {
  tutorialHighlight = h;
  return *this;
}

vector<WCreature> RecruitmentInfo::getAllRecruits(Game* game, CreatureId id) const {
  vector<WCreature> ret;
  if (Collective* col = findEnemy(game))
    ret = filter(col->getCreatures(), [&](WConstCreature c) { return c->getAttributes().getCreatureId() == id; });
  return ret;
}

vector<WCreature> RecruitmentInfo::getAvailableRecruits(Game* game, CreatureId id) const {
  auto ret = getAllRecruits(game, id);
  return getPrefix(ret, max(0, (int)ret.size() - minPopulation));
}

Collective* RecruitmentInfo::findEnemy(Game* game) const {
  for (Collective* col : game->getCollectives())
    if (auto id = col->getEnemyId())
      if (enemyId.contains(*id))
        return col;
  return nullptr;
}
