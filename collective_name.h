#ifndef _COLLECTIVE_NAME_H
#define _COLLECTIVE_NAME_H

#include "util.h"

class CollectiveName {
  public:
  CollectiveName(optional<string> race, optional<string> location, const Creature* leader);
  const string& getShort() const;
  const string& getFull() const;
  const string& getRace() const;

  SERIALIZATION_DECL(CollectiveName);

  private:
  string SERIAL(shortName);
  string SERIAL(fullName);
  string SERIAL(raceName);
};

#endif
