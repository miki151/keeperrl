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

#include "util.h"
#include "name_generator_id.h"

class NameGenerator;

constexpr int maxFirstNameLength = 15;

class CreatureName {
  public:
  CreatureName(const string& name);
  CreatureName(const string& name, const string& plural);
  CreatureName(const char* name);
  void setFirst(optional<string>);
  void generateFirst(NameGenerator*);
  optional<NameGeneratorId> getNameGenerator() const;
  void setStack(const string&);
  void setGroup(const string&);
  void setBare(const string&);
  void addBarePrefix(const string&);
  void addBareSuffix(const string&);
  void useFullTitle(bool = true);
  void setKillTitle(optional<string>);
  const string& stack() const;
  const char* identify() const;
  const optional<string>& stackOnly() const;
  const optional<string>& first() const;
  string firstOrBare() const;
  string bare() const;
  string the() const;
  string a() const;
  string plural() const;
  string multiple(int) const;
  string groupOf(int) const;
  string title() const;
  string aOrTitle() const;

  SERIALIZATION_DECL(CreatureName)

  private:
  string SERIAL(name);
  string SERIAL(pluralName);
  optional<string> SERIAL(stackName);
  optional<string> SERIAL(firstName);
  optional<string> SERIAL(killTitle);
  optional<NameGeneratorId> SERIAL(firstNameGen);
  string SERIAL(groupName) = "group";
  bool SERIAL(fullTitle) = false;
};
