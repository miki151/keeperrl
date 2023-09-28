#include "stdafx.h"
#include "avatar_info.h"
#include "tribe.h"
#include "view.h"
#include "game_config.h"
#include "tribe_alignment.h"
#include "creature_factory.h"
#include "creature.h"
#include "view_object.h"
#include "creature_attributes.h"
#include "gender.h"
#include "creature_name.h"
#include "name_generator.h"
#include "name_generator_id.h"
#include "special_trait.h"
#include "content_factory.h"
#include "options.h"
#include "game_info.h"
#include "unlocks.h"
#include "item.h"

TribeId getPlayerTribeId(TribeAlignment variant) {
  switch (variant) {
    case TribeAlignment::EVIL:
      return TribeId::getDarkKeeper();
    case TribeAlignment::LAWFUL:
      return TribeId::getWhiteKeeper();
  }
}

static ViewObject getUpgradedViewId(const Creature* c) {
  auto object = c->getViewObject();
  if (!c->getAttributes().viewIdUpgrades.empty())
    object.setId(c->getAttributes().viewIdUpgrades.back());
  if (auto it = c->getFirstWeapon())
    object.weaponViewId = it->getEquipedViewId();
  return object;
}

variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View* view,
    const vector<pair<string, KeeperCreatureInfo>>& keeperCreatureInfos,
    ContentFactory* contentFactory, const Unlocks& unlocks) {
  auto& creatureFactory = contentFactory->getCreatures();
  auto keeperCreatures = keeperCreatureInfos.transform([&](auto& elem) {
    return elem.second.creatureId.transform([&](auto& id) {
      auto ret = creatureFactory.fromId(id, getPlayerTribeId(elem.second.tribeAlignment));
      for (auto& trait : elem.second.specialTraits)
        applySpecialTrait(0_global, trait, ret.get(), contentFactory);
      return ret;
    });
  });
  vector<View::AvatarData> keeperAvatarData;
  auto getAllNames = [&] (const PCreature& c) {
    if (auto id = c->getName().getNameGenerator())
      return creatureFactory.getNameGenerator()->getAll(*id);
    else
      return makeVec(c->getName().firstOrBare());
  };
  auto getKeeperFirstNames = [&] (int index) {
    if (auto& nameId = keeperCreatureInfos[index].second.baseNameGen) {
      vector<vector<string>> ret;
      for (auto id : keeperCreatureInfos[index].second.creatureId)
        ret.push_back(creatureFactory.getNameGenerator()->getAll(*nameId));
      return ret;
    } else
      return keeperCreatures[index].transform(getAllNames);
  };
  auto getKeeperName = [&](int index) -> string {
    if (keeperCreatureInfos[index].second.noLeader)
      return keeperCreatures[index][0]->getName().plural();
    return keeperCreatures[index][0]->getName().identify();
  };
  for (int i : All(keeperCreatures))
    keeperAvatarData.push_back(View::AvatarData {
      keeperCreatures[i].transform([](const auto& c) { return string(getName(c->getAttributes().getGender())); }),
      keeperCreatures[i].transform([](const auto& c) { return getUpgradedViewId(c.get()); }),
      getKeeperFirstNames(i),
      keeperCreatureInfos[i].second.tribeAlignment,
      getKeeperName(i),
      keeperCreatureInfos[i].second.description,
      !!keeperCreatureInfos[i].second.baseNameGen,
      !!keeperCreatureInfos[i].second.baseNameGen ? OptionId::SETTLEMENT_NAME : OptionId::PLAYER_NAME,
      !!keeperCreatureInfos[i].second.unlock ? unlocks.isUnlocked(*keeperCreatureInfos[i].second.unlock) : true
    });
  auto result1 = view->chooseAvatar(keeperAvatarData);
  if (auto option = result1.getValueMaybe<AvatarMenuOption>())
    return *option;
  auto result = result1.getReferenceMaybe<View::AvatarChoice>();
  string avatarId;
  PCreature ret;
  optional<string> chosenBaseName;
  auto& creatureInfo = keeperCreatureInfos[result->creatureIndex].second;
  avatarId = keeperCreatureInfos[result->creatureIndex].first;
  ret = std::move(keeperCreatures[result->creatureIndex][result->genderIndex]);
  if (!keeperCreatureInfos[result->creatureIndex].second.noLeader) {
    ret->getName().setBare("Keeper");
    ret->getName().setFirst(result->name);
    ret->getName().useFullTitle();
  } else
    chosenBaseName = result->name;
  CHECK(!creatureInfo.villainGroups.empty());
  return AvatarInfo{std::move(ret), std::move(creatureInfo), avatarId, creatureInfo.tribeAlignment,
      creatureInfo.villainGroups, chosenBaseName };
}

AvatarInfo getQuickGameAvatar(View* view, const vector<pair<string, KeeperCreatureInfo>>& keeperCreatures,
    CreatureFactory* creatureFactory) {
  AvatarInfo ret;
  auto& myKeeper = keeperCreatures[0].second;
  ret.playerCreature = creatureFactory->fromId(myKeeper.creatureId[0], TribeId::getDarkKeeper());
  if (!myKeeper.noLeader)
    ret.playerCreature->getName().setBare("Keeper");
  ret.playerCreature->getName().setFirst("Michal"_s);
  ret.creatureInfo = std::move(myKeeper);
  ret.tribeAlignment = TribeAlignment::EVIL;
  return ret;
}

