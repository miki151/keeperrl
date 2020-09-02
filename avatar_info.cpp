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
#include "name_generator_id.h"
#include "special_trait.h"
#include "content_factory.h"
#include "options.h"

static TribeId getPlayerTribeId(TribeAlignment variant) {
  switch (variant) {
    case TribeAlignment::EVIL:
      return TribeId::getDarkKeeper();
    case TribeAlignment::LAWFUL:
      return TribeId::getAdventurer();
  }
}

static ViewId getUpgradedViewId(const Creature* c) {
  if (!c->getAttributes().viewIdUpgrades.empty())
    return c->getAttributes().viewIdUpgrades.back();
  return c->getViewObject().id();
}

variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View* view, const vector<KeeperCreatureInfo>& keeperCreatureInfos,
    const vector<AdventurerCreatureInfo>& adventurerCreatureInfos, const vector<WarlordInfo>& warlordInfos,
    ContentFactory* contentFactory) {
  auto& creatureFactory = contentFactory->getCreatures();
  auto keeperCreatures = keeperCreatureInfos.transform([&](auto& elem) {
    return elem.creatureId.transform([&](auto& id) {
      auto ret = creatureFactory.fromId(id, getPlayerTribeId(elem.tribeAlignment));
      for (auto& trait : elem.specialTraits)
        applySpecialTrait(0_global, trait, ret.get(), contentFactory);
      return ret;
    });
  });
  auto adventurerCreatures = adventurerCreatureInfos.transform([&](auto& elem) {
    return elem.creatureId.transform([&](auto& id) {
      return creatureFactory.fromId(id, getPlayerTribeId(elem.tribeAlignment));
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
    if (auto& nameId = keeperCreatureInfos[index].baseNameGen) {
      vector<vector<string>> ret;
      for (auto id : keeperCreatureInfos[index].creatureId)
        ret.push_back(creatureFactory.getNameGenerator()->getAll(*nameId));
      return ret;
    } else
      return keeperCreatures[index].transform(getAllNames);
  };
  auto getKeeperName = [&](int index) -> string {
    if (keeperCreatureInfos[index].noLeader)
      return keeperCreatures[index][0]->getName().plural();
    return keeperCreatures[index][0]->getName().identify();
  };
  for (int i : All(keeperCreatures))
    keeperAvatarData.push_back(View::AvatarData {
      keeperCreatures[i].transform([](const auto& c) { return string(getName(c->getAttributes().getGender())); }),
      keeperCreatures[i].transform([](const auto& c) { return getUpgradedViewId(c.get()); }),
      getKeeperFirstNames(i),
      keeperCreatureInfos[i].tribeAlignment,
      getKeeperName(i),
      PlayerRole::KEEPER,
      keeperCreatureInfos[i].description,
      !!keeperCreatureInfos[i].baseNameGen,
      !!keeperCreatureInfos[i].baseNameGen ? OptionId::SETTLEMENT_NAME : OptionId::PLAYER_NAME
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
      adventurerCreatureInfos[i].description,
      false,
      OptionId::PLAYER_NAME
    });
  for (auto& info : warlordInfos)
    adventurerAvatarData.push_back(View::AvatarData {
      {"Warlord"},
      {getUpgradedViewId(info.creatures[0].get())},
      {{info.creatures[0]->getName().firstOrBare()}},
      none,
      info.creatures[0]->getName().identify(),
      PlayerRole::ADVENTURER,
      "Play as a warlord",
      false,
      OptionId::PLAYER_NAME
    });
  auto result1 = view->chooseAvatar(concat(keeperAvatarData, adventurerAvatarData));
  if (auto option = result1.getValueMaybe<AvatarMenuOption>())
    return *option;
  auto result = result1.getReferenceMaybe<View::AvatarChoice>();
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  PCreature ret;
  optional<string> chosenBaseName;
  if (result->creatureIndex < keeperCreatures.size()) {
    creatureInfo = keeperCreatureInfos[result->creatureIndex];
    ret = std::move(keeperCreatures[result->creatureIndex][result->genderIndex]);
    if (!keeperCreatureInfos[result->creatureIndex].noLeader) {
      ret->getName().setBare("Keeper");
      ret->getName().setFirst(result->name);
      ret->getName().useFullTitle();
    } else
      chosenBaseName = result->name;
  } else {
    creatureInfo = adventurerCreatureInfos[result->creatureIndex - keeperCreatures.size()];
    ret = std::move(adventurerCreatures[result->creatureIndex - keeperCreatures.size()][result->genderIndex]);
    ret->getName().setBare("Adventurer");
    ret->getName().setFirst(result->name);
  }
  auto villains = creatureInfo.visit([](const auto& elem) { return elem.tribeAlignment;});
  return AvatarInfo{std::move(ret), std::move(creatureInfo), villains, chosenBaseName };
}

AvatarInfo getQuickGameAvatar(View* view, const vector<KeeperCreatureInfo>& keeperCreatures, CreatureFactory* creatureFactory) {
  AvatarInfo ret;
  auto& myKeeper = keeperCreatures[0];
  ret.playerCreature = creatureFactory->fromId(myKeeper.creatureId[0], TribeId::getDarkKeeper());
  if (!myKeeper.noLeader)
    ret.playerCreature->getName().setBare("Keeper");
  ret.playerCreature->getName().setFirst("Michal"_s);
  ret.creatureInfo = std::move(myKeeper);
  ret.tribeAlignment = TribeAlignment::EVIL;
  return ret;
}

