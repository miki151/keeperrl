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

#ifndef _ENTITY_NAME_H
#define _ENTITY_NAME_H

class EntityName {
  public:
  EntityName(const string& name);
  EntityName(const string& name, const string& plural);
  EntityName(const char* name);
  string bare() const;
  string the() const;
  string a() const;
  string plural() const;
  string multiple(int) const;

  SERIALIZATION_DECL(EntityName);

  private:
  string SERIAL(name);
  string SERIAL(pluralName);
};

#endif

