#include "stdafx.h"
#include "navigation_flags.h"

NavigationFlags::NavigationFlags() : stepOnTile(false), destroy(true), swapPosition(true) {}

NavigationFlags&NavigationFlags::requireStepOnTile() {
  stepOnTile = true;
  return *this;
}

NavigationFlags&NavigationFlags::noDestroying() {
  destroy = false;
  return *this;
}

NavigationFlags&NavigationFlags::noSwapPosition() {
  swapPosition = false;
  return *this;
}
