#include "stdafx.h"
#include "entity_name.h"
#include "util.h"

EntityName::EntityName(const string& n) : name(n), pluralName(name + "s") {
  CHECK(!name.empty());
}

EntityName::EntityName(const char* n) : name(n), pluralName(name + "s") {
  CHECK(!name.empty());
}

EntityName::EntityName(const string& n, const string& p) : name(n), pluralName(p) {
  CHECK(!name.empty());
  CHECK(!pluralName.empty());
}

string EntityName::bare() const {
  return name;
}

string EntityName::the() const {
  if (islower(name[0]))
    return "the " + name;
  return name;
}

string EntityName::a() const {
  if (islower(name[0]))
    return addAParticle(name);
  return name;
}

string EntityName::plural() const {
  return pluralName;
}

string EntityName::multiple(int count) const {
  if (count == 1)
    return name;
  else
    return toString(count) + " " + plural();
}

template <class Archive>
void EntityName::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(name)
    & SVAR(pluralName);
}

SERIALIZABLE(EntityName);
SERIALIZATION_CONSTRUCTOR_IMPL(EntityName);
