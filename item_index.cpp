#include "stdafx.h"
#include "item_index.h"

const char* getName(ItemIndex index, int count) {
  switch (index) {
    case ItemIndex::RANGED_WEAPON: return count == 1 ? "ranged weapon" : "ranged weapons";
    default: FATAL << "Unimplemented";
  }
  return "";
}
