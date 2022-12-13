#pragma once

#include "util.h"

namespace MinionPageElems {
  struct MinionAction {
    int index;
    COMPARE_ALL(index)
  };
  struct Setting {
    int index;
    COMPARE_ALL(index)
  };
  struct Equipment {
    int index;
    COMPARE_ALL(index);
  };
  EMPTY_STRUCT(None);
  using MinionPageIndexVariant = variant<None, MinionAction, Setting, Equipment>;
}

struct MinionPageIndex : public MinionPageElems::MinionPageIndexVariant {
  using MinionPageElems::MinionPageIndexVariant::MinionPageIndexVariant;
  template <typename T>
  void assign(T elem) {
    *((MinionPageElems::MinionPageIndexVariant*)this) = std::move(elem);
  }
  bool left();
  bool right(int numActions, int numSettings);
  bool up(int numActions, int numSettings);
  bool down(int numSettings, int numEquipment);
};