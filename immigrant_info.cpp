#include "stdafx.h"
#include "immigrant_info.h"
#include "furniture.h"
#include "item_index.h"
#include "game.h"
#include "collective.h"
#include "creature.h"
#include "creature_attributes.h"
#include "tutorial.h"
#include "creature_factory.h"
#include "technology.h"
#include "furniture_type.h"
#include "sunlight_info.h"
#include "keybinding.h"
#include "tutorial_highlight.h"
#include "content_factory.h"
#include "special_trait.h"

SERIALIZE_DEF(ImmigrantInfo, NAMED(ids), NAMED(frequency), OPTION(requirements), OPTION(traits), OPTION(spawnLocation), OPTION(groupSize), OPTION(initialRecruitment), OPTION(consumeIds), NAMED(keybinding), NAMED(sound), OPTION(noAuto), NAMED(tutorialHighlight), OPTION(hiddenInHelp), OPTION(invisible), OPTION(specialTraits), OPTION(stripEquipment), OPTION(acceptedAchievement))
SERIALIZATION_CONSTRUCTOR_IMPL(ImmigrantInfo)


SERIALIZE_DEF(AttractionInfo, amountClaimed, types);
SERIALIZATION_CONSTRUCTOR_IMPL(AttractionInfo);

AttractionInfo::AttractionInfo(int cl,  AttractionType a)
  : amountClaimed(cl), types({a}) {}

TString AttractionInfo::getAttractionName(const ContentFactory* contentFactory, const AttractionType& attraction,
    int count) {
  return attraction.match(
      [&](FurnitureType type)->TString {
        return contentFactory->furniture.getData(type).getName();
      },
      [&](ItemIndex index)->TString {
        return *getName(index, count);
      }
  );
}

AttractionInfo::AttractionInfo(int cl, vector<AttractionType> a)
  : amountClaimed(cl), types(a) {}

ImmigrantInfo::ImmigrantInfo(CreatureId id, EnumSet<MinionTrait> t) : ids({id}), traits(t) {}
ImmigrantInfo::ImmigrantInfo(vector<CreatureId> id, EnumSet<MinionTrait> t) : ids(id), traits(t) {
}

STRUCT_IMPL(ImmigrantInfo)

CreatureId ImmigrantInfo::getId(int numCreated) const {
  if (!consumeIds)
    return Random.choose(ids);
  else
    return ids[numCreated];
}

CreatureId ImmigrantInfo::getNonRandomId(int numCreated) const {
  if (!consumeIds)
    return ids[0];
  else
    return ids[numCreated];
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

double ImmigrantInfo::getFrequency() const {
  CHECK(frequency);
  return *frequency;
}

bool ImmigrantInfo::isPersistent() const {
  return !frequency;
}

bool ImmigrantInfo::isInvisible() const {
  return invisible;
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

bool ImmigrantInfo::isHiddenInHelp() const {
  return hiddenInHelp;
}

const vector<SpecialTraitInfo>& ImmigrantInfo::getSpecialTraits() const {
  return specialTraits;
}

ImmigrantInfo& ImmigrantInfo::addRequirement(ImmigrantRequirement t) {
  requirements.push_back({1, t});
  return *this;
}

vector<Creature*> RecruitmentInfo::getAvailableRecruits(Collective* collective, CreatureId id) const {
  auto game = collective->getGame();
  vector<Creature*> ret;
  for (auto col : findEnemy(game))
    if (collective->isKnownVillainLocation(col)) {
      auto creatures = col->getCreatures().filter([&](const Creature* c) {
          return !c->isAffected(LastingEffect::STUNNED) && c->getAttributes().getCreatureId() == id; });
      ret.append(creatures.getPrefix(max(0, (int)creatures.size() - minPopulation)));
    }
  sort(ret.begin(), ret.end(), [](Creature* c1, Creature* c2) {
      return c1->getCombatExperience(false, false) > c2->getCombatExperience(false, false); });
  return ret;
}

vector<Collective*> RecruitmentInfo::findEnemy(Game* game) const {
  vector<Collective*> ret;
  for (Collective* col : game->getCollectives())
    if (auto id = col->getEnemyId())
      if (enemyId.contains(*id))
        ret.push_back(col);
  return ret;
}

#include "pretty_archive.h"
template void ImmigrantInfo::serialize(PrettyInputArchive&, unsigned);
