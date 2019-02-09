#pragma once

struct NavigationFlags {
  NavigationFlags();
  NavigationFlags& requireStepOnTile();
  // This makes the creature stop at the obstacle, and not navigate around it
  NavigationFlags& noDestroying();
  NavigationFlags& noSwapPosition();
  bool stepOnTile;
  bool destroy;
  bool swapPosition;
};
