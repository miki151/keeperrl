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
#include "scripted_ui_data.h"
#include "avatar_menu_option.h"

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
    ContentFactory* contentFactory, const Unlocks& unlocks, Options* options) {
  auto& creatureFactory = contentFactory->getCreatures();
  auto keeperCreatures = keeperCreatureInfos.transform([&](auto& elem) {
    return elem.second.creatureId.transform([&](auto& id) {
      auto ret = creatureFactory.fromId(id, getPlayerTribeId(elem.second.tribeAlignment));
      for (auto& trait : elem.second.specialTraits)
        applySpecialTrait(0_global, trait, ret.get(), contentFactory);
      return ret;
    });
  });
  string keeperBase = keeperCreatureInfos[0].second.baseName;
  int genderIndex = 0;
  auto getFirstName = [&] (const PCreature& c, int keeperBase) {
    if (auto& id = keeperCreatureInfos[keeperBase].second.baseNameGen)
      return creatureFactory.getNameGenerator()->getNext(*id);
    if (auto id = c->getName().getNameGenerator())
      return creatureFactory.getNameGenerator()->getNext(*id);
    else
      return c->getName().firstOrBare();
  };
  string keeperName = getFirstName(keeperCreatures[0][0], 0);
  using RetType = variant<AvatarInfo, AvatarMenuOption>;
  while (true) {
    auto keeperList = ScriptedUIDataElems::List {};
    auto genderList = ScriptedUIDataElems::List {};
    unordered_set<string> addedBases;
    string baseDescription;
    string genderDescription;
    function<bool()> reloadFirstName;
    auto nameOption = OptionId::PLAYER_NAME;
    auto getKeeperName = [&nameOption, &keeperName, options] {
      auto set = options->getValueString(nameOption);
      if (set.empty())
        return keeperName;
      return set;
    };
    for (int i : All(keeperCreatures)) {
      auto baseName = keeperCreatureInfos[i].second.baseName;
      if (!addedBases.count(baseName)) {
        auto data = ScriptedUIDataElems::Record{{
          {"base_name", baseName},
          {"select_callback", ScriptedUIDataElems::Callback{[&keeperBase, baseName, &genderIndex,
              &keeperName, newName = getFirstName(keeperCreatures[i][0], i)]{
            keeperBase = baseName;
            genderIndex = 0;
            keeperName = newName;
            return true;
          }}},
        }};
        if (baseName == keeperBase) {
          data.elems["selected"] = "blabla"_s;
          nameOption = !keeperCreatureInfos[i].second.baseNameGen ? OptionId::PLAYER_NAME : OptionId::SETTLEMENT_NAME;
        }
        keeperList.push_back(std::move(data));
        addedBases.insert(baseName);
      }
      if (baseName == keeperBase)
        for (auto& keeper : keeperCreatures[i]) {
          int selectIndex = genderList.size();
          auto data = ScriptedUIDataElems::Record{{
            {"view_id", getUpgradedViewId(keeper.get()).getViewIdList()},
            {"select_callback", ScriptedUIDataElems::Callback{[&genderIndex, selectIndex, &keeperName,
                newName = getFirstName(keeper, i)]{
              genderIndex = selectIndex;
              keeperName = newName;
              return true;
            }}},
          }};
          if (selectIndex == genderIndex) {
            data.elems["selected"] = "blabla"_s;
            baseDescription = keeperCreatureInfos[i].second.description;
            genderDescription = keeperCreatureInfos[i].second.noLeader
                ? capitalFirst(keeper->getName().plural())
                : capitalFirst(keeper->getName().identify()) + ", " + getName(keeper->getAttributes().getGender());
            reloadFirstName = [&keeperName, options, nameOption, newName = getFirstName(keeper, i)] {
              keeperName = newName;
              options->setValue(nameOption, string());
              return true;
            };
          }
          genderList.push_back(std::move(data));
        }
    }
    optional<RetType> ret;
    auto data = ScriptedUIDataElems::Record{{
      {"gender_list",std::move(genderList)},
      {"keeper_list", std::move(keeperList)},
      {"tutorial_callback", ScriptedUIDataElems::Callback{[&ret]{
        ret = RetType(AvatarMenuOption::TUTORIAL);
        return true;
      }}},
      {"change_mod", ScriptedUIDataElems::Callback{[&ret]{
        ret = RetType(AvatarMenuOption::CHANGE_MOD);
        return true;
      }}},
      {"go_back", ScriptedUIDataElems::Callback{[&ret]{
        ret = RetType(AvatarMenuOption::GO_BACK);
        return true;
      }}},
      {"base_description", baseDescription},
      {"gender_description", genderDescription},
      {nameOption == OptionId::SETTLEMENT_NAME ? "settlement_name" : "first_name", getKeeperName()},
      {"reload_first_name", ScriptedUIDataElems::Callback{std::move(reloadFirstName)}},
      {"edit_first_name", ScriptedUIDataElems::Callback{[getKeeperName, options, view, nameOption] {
        if (auto text = view->getText(
            nameOption == OptionId::SETTLEMENT_NAME ? "Enter settlement name:" : "Enter first name:",
            getKeeperName(), 16))
          options->setValue(nameOption, *text);
        return true;
      }}},
      {"start_game", ScriptedUIDataElems::Callback{[&]{
        PCreature keeper;
        optional<string> chosenBaseName;
        for (int i : All(keeperCreatureInfos)) {
          auto& creatureInfo = keeperCreatureInfos[i].second;
          if (creatureInfo.baseName == keeperBase) {
            if (genderIndex >= creatureInfo.creatureId.size())
              genderIndex -= creatureInfo.creatureId.size();
            else {
              auto avatarId = keeperCreatureInfos[i].first;
              keeper = std::move(keeperCreatures[i][genderIndex]);
              if (!keeperCreatureInfos[i].second.noLeader) {
                keeper->getName().setBare("Keeper");
                keeper->getName().setFirst(getKeeperName());
                keeper->getName().useFullTitle();
              } else
                chosenBaseName = getKeeperName();
              CHECK(!creatureInfo.villainGroups.empty());
              ret = RetType(AvatarInfo{std::move(keeper), std::move(creatureInfo), avatarId, creatureInfo.tribeAlignment,
                  creatureInfo.villainGroups, chosenBaseName });
            }
          }
        }
        return true;
      }}},
    }};
    view->scriptedUI("avatar_menu", data);
    if (ret)
      return *std::move(ret);
  }
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

