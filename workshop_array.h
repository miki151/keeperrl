#pragma once

#include "workshop_item.h"
#include "workshop_type.h"

using WorkshopArray = std::array<vector<WorkshopItemCfg>, EnumInfo<WorkshopType>::size>;
