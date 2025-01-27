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
#include "gender.h"
#include "t_string.h"

TStringId he(Gender g) {
  return get(g, TStringId("HE"), TStringId("SHE"), TStringId("IT"));
}

TStringId his(Gender g) {
  return get(g, TStringId("HIS"), TStringId("HER"), TStringId("ITS"));
}

TStringId him(Gender g) {
  return get(g, TStringId("HIM"), TStringId("HER"), TStringId("IT"));
}

TStringId get(Gender g, TStringId male, TStringId female, TStringId it) {
  switch (g) {
    case Gender::MALE:
      return male;
    case Gender::FEMALE:
      return female;
    case Gender::IT:
      return it;
  }
}

TStringId getName(Gender g) {
  return get(g, TStringId("MALE"), TStringId("FEMALE"), TStringId("GENDERLESS"));
}

TStringId himself(Gender g) {
  return get(g, TStringId("HIMSELF"), TStringId("HERSELF"), TStringId("ITSELF"));
}