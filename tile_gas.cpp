/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"
#include "util.h"
#include "tile_gas.h"
#include "level.h"
#include "game.h"
#include "content_factory.h"
#include "tile_gas_info.h"

SERIALIZE_DEF(TileGas, amount)

double TileGas::getFogVisionCutoff() {
  return 0.2;
}

void TileGas::addAmount(Position pos, TileGasType t, double a) {
  CHECK(a > 0);
  auto prevValue = amount[t];
  amount[t] = min(1., a + amount[t]);
  if (prevValue < getFogVisionCutoff() && amount[t] >= getFogVisionCutoff())
    pos.updateVisibility();
}

void TileGas::tick(Position pos) {
  PROFILE;
  for (auto& elem : amount) {
    auto& value = elem.second;
    auto prevValue = value;
    if (value < 0.1) {
      value = 0;
      continue;
    }
    auto& info = pos.getGame()->getContentFactory()->tileGasTypes.at(elem.first);
    if (info.spread > 0)
      for (Position v : pos.neighbors8(Random)) {
        if (v.canSeeThruIgnoringGas(VisionId::NORMAL) && value > 0 && v.getGasAmount(elem.first) < value) {
          double transfer = pos.getDir(v).isCardinal4() ? info.spread : info.spread / 2;
          transfer = min(value, transfer);
          transfer = min((value - v.getGasAmount(elem.first)) / 2, transfer);
          value -= transfer;
          v.addGas(elem.first, transfer);
        }
      }
    value = max(0.0, value * info.decrease);
    if (prevValue >= getFogVisionCutoff() && value < getFogVisionCutoff())
      pos.updateVisibility();
  }
}

double TileGas::getAmount(TileGasType type) const {
  return getValueMaybe(amount, type).value_or(0.0);
}

