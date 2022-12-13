#include "minion_page_index.h"

namespace E = MinionPageElems;
using namespace E;

bool MinionPageIndex::left() {
  return visit(
      [&](MinionAction a) {
        if (a.index == 0)
          assign(None{});
        else
          assign(MinionAction{a.index - 1});
        return true;
      },
      [&](Setting a) {
        if (a.index == 0)
          assign(None{});
        else
          assign(Setting{a.index - 1});
        return true;
      },
      [&](Equipment e) {
        assign(None{});
        return true;
      },
      [](None) { return false; },
      [](const auto&) { return true;}
  );
}

bool MinionPageIndex::right(int numActions, int numSettings) {
  visit(
    [&](MinionAction a) {
      if (a.index < numActions - 1)
        assign(MinionAction{a.index + 1});
    },
    [&](Setting a) {
      if (a.index < numSettings - 1)
        assign(Setting{a.index + 1});
    },
    [&](None) { assign(MinionAction{0}); },
    [](const auto&) {}
  );
  return true;
}

bool MinionPageIndex::up(int numActions, int numSettings) {
  return visit(
    [&](Setting a) {
      assign(MinionAction{min(numActions - 1, a.index)});
      return true;
    },
    [&](Equipment e) {
      if (e.index > 0)
        assign(Equipment{e.index - 1});
      else
        assign(Setting{numSettings - 1});
      return true;
    },
    [&](None) { return false; },
    [](const auto&) { return true;}
  );
}

bool MinionPageIndex::down(int numSettings, int numEquipment) {
  return visit(
    [&](MinionAction a) {
      assign(Setting{min(numSettings - 1, a.index)});
      return true;
    },
    [&](Setting a) {
      if (numEquipment > 0)
        assign(Equipment{0});
      return true;
    },
    [&](Equipment e) {
      if (e.index < numEquipment - 1)
        assign(Equipment{e.index + 1});
      return true;
    },
    [&](None) { return false; },
    [](const auto&) { return true; }
  );
}
