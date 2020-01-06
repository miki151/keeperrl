#include "stdafx.h"
#include "automaton_slot.h"


const char* getName(AutomatonSlot slot) {
  switch (slot) {
    case AutomatonSlot::ARMS:
      return "arms";
    case AutomatonSlot::HEAD:
      return "head";
    case AutomatonSlot::LEGS:
      return "legs";
    case AutomatonSlot::WINGS:
      return "wings";
  }
}
