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

class CreatureName {
  public:
  CreatureName(const string& name);
  CreatureName(const string& name, const string& plural);
  CreatureName(const char* name);
  void setFirst(const string&);
  void setAdjective(const string&);
  void setJobDescription(const string&);
  void setStack(const string&);
  void setGroup(const string&);
  void useFullTitle();
  const string& stack() const;
  const char* identify() const;
  const optional<string>& stackOnly() const;
  optional<string> first() const;
  optional<string> getAdjective() const;
  optional<string> getJobDescription() const;
  string bare() const;
  string the() const;
  string a() const;
  string plural() const;
  string multiple(int) const;
  string groupOf(int) const;
  string title() const;

  SERIALIZATION_DECL(CreatureName);

  private:
  string SERIAL(name);
  string SERIAL(pluralName);
  optional<string> SERIAL(stackName);
  optional<string> SERIAL(firstName);
  optional<string> SERIAL(adjective);
  optional<string> SERIAL(jobDescription);
  string SERIAL(groupName) = "group";
  bool SERIAL(fullTitle) = false;
};


