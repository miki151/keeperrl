#include "stdafx.h"
#include "automaton_slot.h"


const char* getName(AutomatonSlot slot) {
  switch (slot) {
    case AutomatonSlot::ARMS:
      return "pair of arms";
    case AutomatonSlot::HEAD:
      return "head";
    case AutomatonSlot::LEGS:
      return "pair of legs";
    case AutomatonSlot::WINGS:
      return "pair of wings";
  }
}
