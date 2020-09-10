#pragma once

#include "util.h"
#include "owner_pointer.h"

extern PController getWarlordController(shared_ptr<vector<Creature*>> team, shared_ptr<EnumSet<TeamOrder>> teamOrders);
