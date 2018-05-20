#include "stdafx.h"
#include "creature_name.h"
#include "util.h"

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
  if (fullTitle)
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

string CreatureName::multiple(int count) const {
  if (count == 1)
    return "1 " + name;
  else
    return toString(count) + " " + plural();
}

string CreatureName::groupOf(int count) const {
  if (count == 1)
    return name;
  else
    return groupName + " of " + toString(count) + " " + plural();
}

string CreatureName::title() const {
  string text = name;
  if (jobDescription) text += " " + *jobDescription;
  if (firstName && adjective)
    return *firstName + " the " + *adjective + " " + text;
  else if (firstName)
    return *firstName + " the " + text;
  else if (adjective && !fullTitle)
    return capitalFirst(*adjective) + " " + text;
  else
    return capitalFirst(bare());
}

void CreatureName::setFirst(const string& s) {
  firstName = s;
}

void CreatureName::setAdjective(const string& s) {
  adjective = s;
}

void CreatureName::setJobDescription(const string& s) {
  jobDescription = s;
}

void CreatureName::setStack(const string& s) {
  stackName = s;
}

void CreatureName::setGroup(const string& s) {
  groupName = s;
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

optional<string> CreatureName::first() const {
  return firstName;
}

optional<string> CreatureName::getAdjective() const {
  return adjective;
}

optional<string> CreatureName::getJobDescription() const {
  return jobDescription;
}

void CreatureName::useFullTitle() {
  fullTitle = true;
}

template <class Archive>
void CreatureName::serialize(Archive& ar, const unsigned int version) {
  ar(name, pluralName, stackName, firstName, adjective, jobDescription, groupName, fullTitle);
}

SERIALIZABLE(CreatureName);
SERIALIZATION_CONSTRUCTOR_IMPL(CreatureName);
