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
#include "square.h"


template <class Archive> 
void PoisonGas::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(amount);
  CHECK_SERIAL;
}

SERIALIZABLE(PoisonGas);

void PoisonGas::addAmount(double a) {
  CHECK(a > 0);
  amount = min(1., a + amount);
}

const double decrease = 0.98;
const double spread = 0.10;

void PoisonGas::tick(Level* level, Vec2 pos) {
  if (amount < 0.1) {
    amount = 0;
    return;
  }
  for (Square* square : level->getSquares(pos.neighbors8(true))) {
    if (square->canSeeThru() && amount > 0 && square->getPoisonGasAmount() < amount) {
      double transfer = (square->getPosition() - pos).isCardinal4() ? spread : spread / 2;
      transfer = min(amount, transfer);
      transfer = min((amount - square->getPoisonGasAmount()) / 2, transfer);
      amount -= transfer;
      square->addPoisonGas(transfer);
    }
  }
  amount = max(0.0, amount * decrease);
}

double PoisonGas::getAmount() const {
  return amount;
}

