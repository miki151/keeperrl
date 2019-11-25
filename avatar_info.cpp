#include "stdafx.h"
#include "avatar_info.h"
#include "tribe.h"
#include "view.h"
#include "game_config.h"
#include "tribe_alignment.h"
#include "creature_factory.h"
#include "player_role.h"
#include "creature.h"
#include "view_object.h"
#include "creature_attributes.h"
#include "gender.h"
#include "creature_name.h"
#include "name_generator.h"
#include "special_trait.h"

static TribeId getPlayerTribeId(TribeAlignment variant) {
  switch (variant) {
    case TribeAlignment::EVIL:
      return TribeId::getDarkKeeper();
    case TribeAlignment::LAWFUL:
      return TribeId::getAdventurer();
  }
}

static vector<ViewId> getUpgradedViewId(const Creature* c) {
  vector<ViewId> ret {c->getViewObject().id() };
  for (auto& u : c->getAttributes().viewIdUpgrades)
    ret.push_back(u);
  return ret;
}

variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View* view, const PlayerCreaturesInfo* creatures, Options* options,
    CreatureFactory* creatureFactory) {
  auto keeperCreatureInfos = creatures->first;
  auto keeperCreatures = keeperCreatureInfos.transform([&](auto& elem) {
    return elem.creatureId.transform([&](auto& id) {
      auto ret = creatureFactory->fromId(id, getPlayerTribeId(elem.tribeAlignment));
      for (auto& trait : elem.specialTraits)
        applySpecialTrait(trait, ret.get());
      return ret;
    });
  });
  auto adventurerCreatureInfos = creatures->second;
  auto adventurerCreatures = adventurerCreatureInfos.transform([&](auto& elem) {
    return elem.creatureId.transform([&](auto& id) {
      return creatureFactory->fromId(id, getPlayerTribeId(elem.tribeAlignment));
    });
  });
  vector<View::AvatarData> keeperAvatarData;
  auto getAllNames = [&] (const PCreature& c) {
    if (auto id = c->getName().getNameGenerator())
      return creatureFactory->getNameGenerator()->getAll(*id);
    else
      return makeVec(c->getName().firstOrBare());
  };
  for (int i : All(keeperCreatures))
    keeperAvatarData.push_back(View::AvatarData {
      keeperCreatures[i].transform([](const auto& c) { return string(getName(c->getAttributes().getGender())); }),
      keeperCreatures[i].transform([](const auto& c) { return getUpgradedViewId(c.get()); }),
      keeperCreatures[i].transform(getAllNames),
      keeperCreatureInfos[i].tribeAlignment,
      keeperCreatures[i][0]->getName().identify(),
      PlayerRole::KEEPER,
      keeperCreatureInfos[i].description
    });
  vector<View::AvatarData> adventurerAvatarData;
  for (int i : All(adventurerCreatures))
    adventurerAvatarData.push_back(View::AvatarData {
      adventurerCreatures[i].transform([](const auto& c) { return string(getName(c->getAttributes().getGender())); }),
      adventurerCreatures[i].transform([](const auto& c) { return getUpgradedViewId(c.get()); }),
      adventurerCreatures[i].transform(getAllNames),
      adventurerCreatureInfos[i].tribeAlignment,
      adventurerCreatures[i][0]->getName().identify(),
      PlayerRole::ADVENTURER,
      adventurerCreatureInfos[i].description
    });
  auto result1 = view->chooseAvatar(concat(keeperAvatarData, adventurerAvatarData));
  if (auto option = result1.getValueMaybe<AvatarMenuOption>())
    return *option;
  auto result = result1.getReferenceMaybe<View::AvatarChoice>();
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  PCreature ret;
  if (result->creatureIndex < keeperCreatures.size()) {
    creatureInfo = creatures->first[result->creatureIndex];
    ret = std::move(keeperCreatures[result->creatureIndex][result->genderIndex]);
    if (!creatures->first[result->creatureIndex].noLeader)
      ret->getName().setBare("Keeper");
  } else {
    creatureInfo = creatures->second[result->creatureIndex - keeperCreatures.size()];
    ret = std::move(adventurerCreatures[result->creatureIndex - keeperCreatures.size()][result->genderIndex]);
    ret->getName().setBare("Adventurer");
  }
  ret->getName().setFirst(result->name);
  auto villains = creatureInfo.visit([](const auto& elem) { return elem.tribeAlignment;});
  return AvatarInfo{std::move(ret), std::move(creatureInfo), villains };
}

AvatarInfo getQuickGameAvatar(View* view, const PlayerCreaturesInfo* creatures, CreatureFactory* creatureFactory) {
  auto keeperCreatures = creatures->first;
  AvatarInfo ret;
  ret.playerCreature = creatureFactory->fromId(keeperCreatures[3].creatureId[0], TribeId::getDarkKeeper());
  if (!keeperCreatures[3].noLeader)
    ret.playerCreature->getName().setBare("Keeper");
  ret.creatureInfo = std::move(keeperCreatures[3]);
  ret.tribeAlignment = TribeAlignment::EVIL;
  return ret;
}

