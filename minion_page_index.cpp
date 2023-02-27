#include "minion_page_index.h"

bool MinionPageIndex::left() {
  using namespace MinionPageElems;
  return visit(
      [&](MinionPageElems::MinionAction a) {
        if (a.index == 0)
          assign(None{});
        else
          assign(MinionPageElems::MinionAction{a.index - 1});
        return true;
      },
      [&](Setting a) {
        if (a.index == 0)
          assign(None{});
        else
          assign(Setting{a.index - 1});
        return true;
      },
      [&](MinionPageElems::Equipment e) {
        assign(None{});
        return true;
      },
      [](None) { return false; },
      [](const auto&) { return true;}
  );
}

bool MinionPageIndex::right(int numActions, int numSettings) {
  using namespace MinionPageElems;
  visit(
    [&](MinionPageElems::MinionAction a) {
      if (a.index < numActions - 1)
        assign(MinionPageElems::MinionAction{a.index + 1});
    },
    [&](Setting a) {
      if (a.index < numSettings - 1)
        assign(Setting{a.index + 1});
    },
    [&](None) { assign(MinionPageElems::MinionAction{0}); },
    [](const auto&) {}
  );
  return true;
}

bool MinionPageIndex::up(int numActions, int numSettings) {
  using namespace MinionPageElems;
  return visit(
    [&](Setting a) {
      assign(MinionPageElems::MinionAction{min(numActions - 1, a.index)});
      return true;
    },
    [&](MinionPageElems::Equipment e) {
      if (e.index > 0)
        assign(MinionPageElems::Equipment{e.index - 1});
      else
        assign(Setting{numSettings - 1});
      return true;
    },
    [&](None) { return false; },
    [](const auto&) { return true;}
  );
}

bool MinionPageIndex::down(int numSettings, int numEquipment) {
  using namespace MinionPageElems;
  return visit(
    [&](MinionPageElems::MinionAction a) {
      assign(Setting{min(numSettings - 1, a.index)});
      return true;
    },
    [&](Setting a) {
      if (numEquipment > 0)
        assign(MinionPageElems::Equipment{0});
      return true;
    },
    [&](MinionPageElems::Equipment e) {
      if (e.index < numEquipment - 1)
        assign(MinionPageElems::Equipment{e.index + 1});
      return true;
    },
    [&](None) { return false; },
    [](const auto&) { return true; }
  );
}
