#include "stdafx.h"
#include "creature_view.h"
#include "level.h"

template <class Archive> 
void CreatureView::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(visibleEnemies);
  CHECK_SERIAL;
}

SERIALIZABLE(CreatureView);

void CreatureView::updateVisibleEnemies() {
  visibleEnemies.clear();
  for (const Creature* c : getLevel()->getAllCreatures()) 
    if (isEnemy(c) && (canSee(c)))
      visibleEnemies.push_back(c);
  for (const Creature* c : getUnknownAttacker())
    if (!contains(visibleEnemies, c))
      visibleEnemies.push_back(c);
}

vector<const Creature*> CreatureView::getVisibleEnemies() const {
  return visibleEnemies;
}

