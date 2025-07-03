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
  int keeperBase = 0;
  int genderIndex = 0;
  auto getFirstName = [&] (const PCreature& c, int keeperBase) -> optional<string> {
    if (auto& id = keeperCreatureInfos[keeperBase].second.baseNameGen)
      return creatureFactory.getNameGenerator()->getNext(*id);
    if (auto id = c->getName().getNameGenerator())
      return creatureFactory.getNameGenerator()->getNext(*id);
    else
      return none;
  };
  auto keeperName = getFirstName(keeperCreatures[0][0], 0);
  using RetType = variant<AvatarInfo, AvatarMenuOption>;
  while (true) {
    auto keeperList = ScriptedUIDataElems::List {};
    auto genderList = ScriptedUIDataElems::List {};
    HashSet<TString> addedBases;
    TString baseDescription;
    TString genderDescription;
    function<bool()> reloadFirstName;
    auto nameOption = OptionId::PLAYER_NAME;
    auto getKeeperName = [&nameOption, &keeperName, options] () -> optional<string> {
      auto set = options->getValueString(nameOption);
      if (set.empty() || !keeperName)
        return keeperName;
      return set;
    };
    auto keeperUnlocked = [&keeperCreatureInfos, unlocks](int index) {
      return !keeperCreatureInfos[index].second.unlock ||
          unlocks.isUnlocked(*keeperCreatureInfos[index].second.unlock);
    };
    for (int i : All(keeperCreatures)) {
      auto baseName = keeperCreatureInfos[i].second.baseName;
      auto baseUnlocked = [&] {
        for (int index : All(keeperCreatureInfos))
          if (keeperCreatureInfos[index].second.baseName == baseName && keeperUnlocked(index))
            return true;
        return false;
      }();
      if (!addedBases.count(baseName)) {
        auto data = ScriptedUIDataElems::Record{{
          {"base_name", baseName}
        }};
        if (baseUnlocked)
          data.elems["select_callback"] = ScriptedUIDataElems::Callback{[&keeperBase, i, baseName, &genderIndex,
              &keeperName, newName = getFirstName(keeperCreatures[i][0], i), &keeperCreatureInfos] {
            if (keeperCreatureInfos[keeperBase].second.baseName != baseName) {
              keeperBase = i;
              genderIndex = 0;
              keeperName = newName;
            }
            return true;
          }};
        else
          data.elems["locked"] = TString("blabla"_s);
        if (baseName == keeperCreatureInfos[keeperBase].second.baseName) {
          data.elems["selected"] = TString("blabla"_s);
          nameOption = !keeperCreatureInfos[i].second.baseNameGen ? OptionId::PLAYER_NAME : OptionId::SETTLEMENT_NAME;
        }
        keeperList.push_back(std::move(data));
        addedBases.insert(baseName);
      }
      if (baseName == keeperCreatureInfos[keeperBase].second.baseName) {
        for (int gender : All(keeperCreatures[i])) {
          auto& keeper = keeperCreatures[i][gender];
          auto data = ScriptedUIDataElems::Record{};
          if (keeperUnlocked(i)) {
            data.elems["view_id"] = getUpgradedViewId(keeper.get()).getViewIdList();
            data.elems["select_callback"] = ScriptedUIDataElems::Callback{[&genderIndex, gender, &keeperName,
                &keeperBase, i, newName = getFirstName(keeper, i)]{
              genderIndex = gender;
              keeperName = newName;
              keeperBase = i;
              return true;
            }};
          } else
            data.elems["view_id"] = vector<ViewId>(1, ViewId("unknown_monster", Color::GRAY));
          if (gender == genderIndex && i == keeperBase) {
            data.elems["selected"] = TString("blabla"_s);
            baseDescription = keeperCreatureInfos[i].second.description;
            genderDescription = capitalFirst(keeperCreatureInfos[i].second.noLeader
                ? keeper->getName().plural()
                : keeper->getName().name);
            reloadFirstName = [&keeperName, options, nameOption, newName = getFirstName(keeper, i)] {
              keeperName = newName;
              options->setValue(nameOption, string());
              return true;
            };
          }
          genderList.push_back(std::move(data));
        }
      }
    }
    optional<RetType> ret;
    auto keeperBaseInc = [&keeperCreatureInfos, &keeperBase, keeperUnlocked, &genderIndex, &keeperName,
        &keeperCreatures, &getFirstName] (int dir) {
      auto origName = keeperCreatureInfos[keeperBase].second.baseName;
      auto nextName = [&] {
        for (int i = (keeperBase + dir + keeperCreatures.size()) % keeperCreatures.size(); i != keeperBase;
            i = (i + dir + keeperCreatures.size()) % keeperCreatures.size())
          if (keeperCreatureInfos[i].second.baseName != origName && keeperUnlocked(i))
            return keeperCreatureInfos[i].second.baseName;
        fail();
      }();
      for (int i : All(keeperCreatureInfos))
        if (keeperCreatureInfos[i].second.baseName == nextName) {
          keeperBase = i;
          break;
        }
      genderIndex = 0;
      keeperName = getFirstName(keeperCreatures[keeperBase][0], keeperBase);
      return true;
    };
    auto genderInc = [&keeperCreatureInfos, &keeperBase, &genderIndex, keeperUnlocked, &keeperName,
        &keeperCreatures, &getFirstName] (int dir) {
      genderIndex += dir;
      if (genderIndex >= keeperCreatures[keeperBase].size() || genderIndex < 0) {
        auto origName = keeperCreatureInfos[keeperBase].second.baseName;
        do {
          keeperBase = (keeperBase + dir + keeperCreatureInfos.size()) % keeperCreatureInfos.size();
        } while (keeperCreatureInfos[keeperBase].second.baseName != origName || !keeperUnlocked(keeperBase));
        genderIndex = dir > 0 ? 0 : keeperCreatures[keeperBase].size() - 1;
      }
      keeperName = getFirstName(keeperCreatures[keeperBase][genderIndex], keeperBase);
      return true;
    };
    auto data = ScriptedUIDataElems::Record{{
      {"gender_list",std::move(genderList)},
      {"keeper_list", std::move(keeperList)},
      {"base_inc_callback", ScriptedUIDataElems::Callback{[&keeperBaseInc]{ return keeperBaseInc(1); }}},
      {"base_dec_callback", ScriptedUIDataElems::Callback{[&keeperBaseInc]{ return keeperBaseInc(-1); }}},
      {"gender_inc_callback", ScriptedUIDataElems::Callback{[&genderInc]{ return genderInc(1); }}},
      {"gender_dec_callback", ScriptedUIDataElems::Callback{[&genderInc]{ return genderInc(-1); }}},
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

      {"start_game", ScriptedUIDataElems::Callback{[&]{
        PCreature keeper;
        optional<string> chosenBaseName;
        auto avatarId = keeperCreatureInfos[keeperBase].first;
        keeper = std::move(keeperCreatures[keeperBase][genderIndex]);
        auto& creatureInfo = keeperCreatureInfos[keeperBase].second;
        if (!creatureInfo.noLeader) {
          if (creatureInfo.baseName != TStringId("ADVENTURER")) {
            keeper->getName().setBare(get(keeper->getAttributes().getGender(), TStringId("KEEPER"),
                TStringId("KEEPER_F"), TStringId("KEEPER")));
            keeper->getName().useFullTitle();
          }
          keeper->getName().setFirst(getKeeperName());
        } else
          chosenBaseName = getKeeperName();
        CHECK(!creatureInfo.villainGroups.empty());
        ret = RetType(AvatarInfo{std::move(keeper), std::move(creatureInfo), avatarId, creatureInfo.tribeAlignment,
            creatureInfo.villainGroups, chosenBaseName });
        return true;
      }}},
    }};
    if (auto name = getKeeperName()) {
      data.elems[nameOption == OptionId::SETTLEMENT_NAME ? "settlement_name" : "first_name"] = TString(*name);
      data.elems["reload_first_name"] = ScriptedUIDataElems::Callback{std::move(reloadFirstName)};
      data.elems["edit_first_name"] = ScriptedUIDataElems::Callback{[name, options, view, nameOption] {
          if (auto text = view->getText(
              nameOption == OptionId::SETTLEMENT_NAME ? TStringId("ENTER_SETTLEMENT_NAME") : TStringId("ENTER_FIRST_NAME"),
              *name, 16))
            options->setValue(nameOption, *text);
          return true;
        }};
    }
    ScriptedUIState state;
    state.exit = ScriptedUIDataElems::Callback{[&ret] {
      ret = RetType(AvatarMenuOption::GO_BACK);
      return true;
    }};
    view->scriptedUI("avatar_menu", data, state);
    if (ret)
      return std::move(*ret);
  }
}

AvatarInfo getQuickGameAvatar(View* view, const KeeperCreatureInfo& myKeeper,
    CreatureFactory* creatureFactory) {
  AvatarInfo ret;
  ret.playerCreature = creatureFactory->fromId(myKeeper.creatureId[0], TribeId::getDarkKeeper());
  if (!myKeeper.noLeader)
    ret.playerCreature->getName().setBare(TStringId("KEEPER"));
  ret.playerCreature->getName().setFirst("Jollibrond"_s);
  ret.creatureInfo = std::move(myKeeper);
  ret.tribeAlignment = myKeeper.tribeAlignment;
  return ret;
}

