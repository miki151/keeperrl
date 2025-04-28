#include "stdafx.h"
#include "creature_name.h"
#include "util.h"
#include "name_generator.h"

CreatureName::CreatureName(const string& n) : name(n), pluralName(name + "s") {
  CHECK(!name.empty());
}

CreatureName::CreatureName(const char* n) : CreatureName(string(n)) {
}

CreatureName::CreatureName(const string& n, const string& p) : name(n), pluralName(p) {
  CHECK(!name.empty());
  CHECK(!pluralName.empty());
}

const char* CreatureName::identify() const {
  return name.c_str();
}

string CreatureName::bare() const {
  if (fullTitle && firstName)
    return title();
  else
    return name;
}

string CreatureName::the() const {
  if (fullTitle)
    return title();
  else
    return "the " + name;
}

string CreatureName::a() const {
  if (fullTitle)
    return title();
  else
    return addAParticle(name);
}

string CreatureName::plural() const {
  return pluralName;
}

string CreatureName::groupOf(int count) const {
  if (count == 1)
    return name;
  else
    return groupName + " of " + toString(count) + " " + plural();
}

string CreatureName::title() const {
  if (firstName) {
    if (killTitle)
      return *firstName + " the " + *killTitle;
    else
      return *firstName + " the " + name;
  } else {
    if (killTitle)
      return capitalFirst(bare()) + " the " + *killTitle;
    else
      return capitalFirst(bare());
  }
}

string CreatureName::aOrTitle() const {
  if (firstName) {
    if (killTitle)
      return *firstName + " the " + *killTitle;
    else
      return *firstName + " the " + name;
  } else {
    if (killTitle)
      return capitalFirst(bare()) + " the " + *killTitle;
    else
      return a();
  }
}

void CreatureName::setFirst(optional<string> s) {
  firstName = std::move(s);
}

void CreatureName::generateFirst(NameGenerator* generator) {
  if (firstNameGen)
    firstName = generator->getNext(*firstNameGen);
}

optional<NameGeneratorId> CreatureName::getNameGenerator() const {
  return firstNameGen;
}

void CreatureName::setStack(const string& s) {
  stackName = s;
}

void CreatureName::setGroup(const string& s) {
  groupName = s;
}

void CreatureName::setBare(const std::string& s) {
  name = s;
}

void CreatureName::addBarePrefix(const string& p) {
  if (!stackName)
    stackName = name;
  name = p + " " + name;
}

void CreatureName::addBareSuffix(const string& p) {
  if (!stackName)
    stackName = name;
  name = name + " " + p;
}

const optional<string>& CreatureName::stackOnly() const {
  return stackName;
}

const string& CreatureName::stack() const {
  if (stackName)
    return *stackName;
  else
    return name;
}

const optional<string>& CreatureName::first() const {
  return firstName;
}

string CreatureName::firstOrBare() const {
  return firstName.value_or(capitalFirst(bare()));
}

void CreatureName::useFullTitle(bool b) {
  fullTitle = b;
}

void CreatureName::setKillTitle(optional<string> t) {
  killTitle = std::move(t);
}

template <class Archive>
void CreatureName::serialize(Archive& ar, const unsigned int version) {
  ar(name, pluralName, stackName, firstName, groupName, fullTitle, firstNameGen, killTitle);
}

SERIALIZABLE(CreatureName);
SERIALIZATION_CONSTRUCTOR_IMPL(CreatureName);

#include "pretty_archive.h"

template<>
void CreatureName::serialize(PrettyInputArchive& ar1, unsigned) {
  ar1(NAMED(name), OPTION(pluralName), NAMED(stackName), NAMED(firstNameGen), NAMED(firstName), OPTION(groupName), OPTION(fullTitle), endInput());
  if (pluralName.empty())
    pluralName = name + "s";
}

