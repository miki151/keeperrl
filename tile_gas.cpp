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
  auto prevValue = amount[t].total;
  amount[t].total = min(1., a + amount[t].total);
  if (prevValue < getFogVisionCutoff() && amount[t].total >= getFogVisionCutoff()) {
    pos.updateVisibility();
    pos.updateConnectivity();
  }
}

void TileGas::addPermanentAmount(TileGasType t, double a) {
  auto& values = amount[t];
  values.total = min(1.0, values.total + a);
  values.permanent = min(1.0, values.permanent + a);
}

bool TileGas::hasSunlightBlockingAmount() const {
  for (auto& elem : amount)
    if (elem.second.total > getFogVisionCutoff())
      return true;
  return false;
}

void TileGas::tick(Position pos) {
  PROFILE;
  for (auto& elem : amount) {
    auto& value = elem.second;
    auto prevValue = value.total;
    auto& info = pos.getGame()->getContentFactory()->tileGasTypes.at(elem.first);
    if (info.effect && value.total > 0.01 && Random.chance(value.total))
      info.effect->apply(pos);
    if (value.total - value.permanent < 0.1)
      value.total = value.permanent;
    else {
      if (info.spread > 0)
        for (Position v : pos.neighbors8(Random)) {
          if (v.canSeeThruIgnoringGas(VisionId::NORMAL) && value.total - value.permanent > 0 &&
              v.getGasAmount(elem.first) < value.total) {
            double transfer = pos.getDir(v).isCardinal4() ? info.spread : info.spread / 2;
            transfer = min(value.total - value.permanent, transfer);
            transfer = min((value.total - v.getGasAmount(elem.first)) / 2, transfer);
            value.total -= transfer;
            CHECK(value.total >= value.permanent);
            v.addGas(elem.first, transfer);
          }
        }
      value.total = max(value.permanent, value.permanent + (value.total - value.permanent) * info.decrease);
    }
    if (prevValue >= getFogVisionCutoff() && value.total < getFogVisionCutoff()) {
      pos.updateVisibility();
      pos.updateConnectivity();
    }
  }
}

double TileGas::getAmount(TileGasType type) const {
  if (auto res = getReferenceMaybe(amount, type))
    return res->total;
  return 0;
}
