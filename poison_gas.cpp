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

#include "poison_gas.h"
#include "level.h"

SERIALIZE_DEF(PoisonGas, amount)

void PoisonGas::addAmount(double a) {
  CHECK(a > 0);
  amount = min(1., a + amount);
}

const double decrease = 0.98;
const double spread = 0.10;

void PoisonGas::tick(Position pos) {
  if (amount < 0.01) {
    amount = 0;
    return;
  }
  for (Position v : pos.neighbors8(Random)) {
    if (v.canSeeThru(VisionId::NORMAL) && amount > 0 && v.getPoisonGasAmount() < amount) {
      double transfer = pos.getDir(v).isCardinal4() ? spread : spread / 2;
      transfer = min(amount, transfer);
      transfer = min((amount - v.getPoisonGasAmount()) / 2, transfer);
      amount -= transfer;
      v.addPoisonGas(transfer);
    }
  }
  amount = max(0.0, amount * decrease);
}

double PoisonGas::getAmount() const {
  return amount;
}

