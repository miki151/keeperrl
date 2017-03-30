#pragma once

#include "util.h"

class CollectiveName {
  public:
  CollectiveName(optional<string> race, optional<string> location, WConstCreature leader);
  const string& getShort() const;
  const string& getFull() const;
  const string& getRace() const;

  SERIALIZATION_DECL(CollectiveName);

  private:
  string SERIAL(shortName);
  string SERIAL(fullName);
  string SERIAL(raceName);
};

