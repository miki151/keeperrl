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

#include "tile_gas.h"
#include "level.h"
#include "util.h"

SERIALIZE_DEF(TileGas, amount)

void TileGas::addAmount(TileGasType t, double a) {
  CHECK(a > 0);
  amount[t] = min(1., a + amount[t]);
}


static double getGasSpread(TileGasType type) {
  switch (type) {
    case TileGasType::FOG: return 0.01;
    case TileGasType::POISON: return 0.1;
  }
}
const double decrease = 0.98;

double TileGas::getFogVisionCutoff() {
  return 0.2;
}

void TileGas::tick(Position pos) {
  PROFILE;
  for (auto type : ENUM_ALL(TileGasType)) {
    auto& value = amount[type];
    if (value < 0.01) {
      value = 0;
      continue;
    }
    const auto spread = getGasSpread(type);
    for (Position v : pos.neighbors8(Random)) {
      if (v.canSeeThruIgnoringGas(VisionId::NORMAL) && value > 0 && v.getGasAmount(type) < value) {
        double transfer = pos.getDir(v).isCardinal4() ? spread : spread / 2;
        transfer = min(value, transfer);
        transfer = min((value - v.getGasAmount(type)) / 2, transfer);
        value -= transfer;
        v.addGas(type, transfer);
      }
    }
    value = max(0.0, value * decrease);
  }
}

double TileGas::getAmount(TileGasType type) const {
  return amount[type];
}

