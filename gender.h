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

#pragma once

#include "stdafx.h"
#include "serialization.h"

class Gender {
  public:
  const static Gender male;
  const static Gender female;
  static void initialize();

  const char* he() const;
  const char* his() const;
  const char* him() const;
  const char* god() const;
  const char* sireOrDame() const;


  SERIALIZATION_DECL(Gender);

  bool operator == (const Gender&) const;
  bool operator != (const Gender&) const;

  private:
  Gender(bool female);
  bool SERIAL(fem);
};

