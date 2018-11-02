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

using PlayerCreaturesInfo = pair<vector<KeeperCreatureInfo>, vector<AdventurerCreatureInfo>>;

static PlayerCreaturesInfo readKeeperCreaturesConfig(View* view, GameConfig* config) {
  PlayerCreaturesInfo elem;
  while (1) {
    if (auto error = config->readObject(elem, GameConfigId::PLAYER_CREATURES))
      view->presentText("Error reading player creatures definition file", *error);
    else
      break;
  }
  return elem;
}

static TribeId getPlayerTribeId(TribeAlignment variant) {
  switch (variant) {
    case TribeAlignment::EVIL:
      return TribeId::getDarkKeeper();
    case TribeAlignment::LAWFUL:
      return TribeId::getAdventurer();
  }
}

variant<AvatarInfo, AvatarMenuOption> getAvatarInfo(View* view, GameConfig* gameConfig) {
  auto keeperCreatureInfos = readKeeperCreaturesConfig(view, gameConfig).first;
  auto keeperCreatures = keeperCreatureInfos.transform([](auto& elem) {
      return CreatureFactory::fromId(elem.creatureId, getPlayerTribeId(elem.tribeAlignment));
  });
  auto adventurerCreatureInfos = readKeeperCreaturesConfig(view, gameConfig).second;
  auto adventurerCreatures = adventurerCreatureInfos.transform([](auto& elem) {
      return CreatureFactory::fromId(elem.creatureId, getPlayerTribeId(elem.tribeAlignment));
  });
  vector<View::AvatarData> keeperAvatarData;
  for (int i : All(keeperCreatures))
    keeperAvatarData.push_back(View::AvatarData {
      keeperCreatures[i]->getViewObject().id(),
      keeperCreatures[i]->getAttributes().getGender(),
      keeperCreatureInfos[i].tribeAlignment,
      keeperCreatures[i]->getName().identify(),
      PlayerRole::KEEPER,
      keeperCreatureInfos[i].description
    });
  vector<View::AvatarData> adventurerAvatarData;
  for (int i : All(adventurerCreatures))
    adventurerAvatarData.push_back(View::AvatarData {
      adventurerCreatures[i]->getViewObject().id(),
      adventurerCreatures[i]->getAttributes().getGender(),
      adventurerCreatureInfos[i].tribeAlignment,
      adventurerCreatures[i]->getName().identify(),
      PlayerRole::ADVENTURER,
      adventurerCreatureInfos[i].description
    });
  auto result1 = view->chooseAvatar(concat(keeperAvatarData, adventurerAvatarData));
  if (auto option = result1.getReferenceMaybe<AvatarMenuOption>())
    return *option;
  auto result = *result1.getValueMaybe<int>();
  variant<KeeperCreatureInfo, AdventurerCreatureInfo> creatureInfo;
  PCreature ret;
  if (result < keeperCreatures.size()) {
    creatureInfo = readKeeperCreaturesConfig(view, gameConfig).first[result];
    ret = std::move(keeperCreatures[result]);
  } else {
    ret = std::move(adventurerCreatures[result - keeperCreatures.size()]);
    creatureInfo = readKeeperCreaturesConfig(view, gameConfig).second[result - keeperCreatures.size()];
  }
  auto villains = creatureInfo.visit([](const auto& elem) { return elem.tribeAlignment;});
  return AvatarInfo{std::move(ret), creatureInfo, villains };
}

AvatarInfo getQuickGameAvatar(View* view, GameConfig* gameConfig) {
  auto keeperCreatures = readKeeperCreaturesConfig(view, gameConfig).first;
  AvatarInfo ret;
  ret.playerCreature = CreatureFactory::fromId(keeperCreatures[0].creatureId, TribeId::getDarkKeeper());
  ret.creatureInfo = keeperCreatures[0];
  ret.tribeAlignment = TribeAlignment::EVIL;
  return ret;
}

