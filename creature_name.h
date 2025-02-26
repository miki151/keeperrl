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
#include "t_string.h"

class NameGenerator;

constexpr int maxFirstNameLength = 15;

class CreatureName {
  public:
  CreatureName(const TString& name);
  CreatureName(const TString& name, const TString& plural);
  void setFirst(optional<string>);
  void generateFirst(NameGenerator*);
  optional<NameGeneratorId> getNameGenerator() const;
  void setStack(const TString&);
  void setBare(const TString&);
  void modifyName(TStringId);
  void useFullTitle(bool = true);
  void setKillTitle(optional<TString>);
  const TString& stack() const;
  const char* identify() const;
  const optional<TString>& stackOnly() const;
  const optional<string>& first() const;
  TString firstOrBare() const;
  TString bare() const;
  TString the() const;
  TString a() const;
  TString plural() const;
  TString getGroupName() const;
  TString title() const;
  TString aOrTitle() const;

  SERIALIZATION_DECL(CreatureName)

  TString SERIAL(name);

  private:
  TString SERIAL(pluralName);
  optional<TString> SERIAL(stackName);
  optional<string> SERIAL(firstName);
  optional<TString> SERIAL(killTitle);
  optional<NameGeneratorId> SERIAL(firstNameGen);
  TString SERIAL(groupName) = TStringId("GROUP");
  bool SERIAL(fullTitle) = false;
};
