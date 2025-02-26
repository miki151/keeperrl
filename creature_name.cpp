#include "stdafx.h"
#include "creature_name.h"
#include "util.h"
#include "name_generator.h"
#include "t_string.h"

CreatureName::CreatureName(const TString& n) : name(n) {
  CHECK(!name.empty());
}

CreatureName::CreatureName(const TString& n, const TString& p) : name(n), pluralName(p) {
  CHECK(!name.empty());
  CHECK(!pluralName.empty());
}

const char* CreatureName::identify() const {
  return name.text.visit(
    [](const TSentence& elem) { return elem.id.data(); },
    [](const string& elem) { return elem.data(); }
  );
}

TString CreatureName::bare() const {
  if (fullTitle && firstName)
    return title();
  else
    return name;
}

TString CreatureName::the() const {
  if (fullTitle)
    return title();
  else
    return TSentence("THE_ARTICLE", name);
}

TString CreatureName::a() const {
  if (fullTitle)
    return title();
  else
    return TSentence("A_ARTICLE", name);
}

TString CreatureName::plural() const {
  return pluralName;
}

TString CreatureName::getGroupName() const {
  return groupName;
}

TString CreatureName::title() const {
  if (firstName)
    return TSentence("CREATURE_TITLE", TString(*firstName), killTitle ? *killTitle : name);
  else {
    if (killTitle)
      return TSentence("CREATURE_TITLE", capitalFirst(bare()), *killTitle);
    else
      return capitalFirst(bare());
  }
}

TString CreatureName::aOrTitle() const {
  if (firstName)
    return TSentence("CREATURE_TITLE", TString(*firstName), killTitle ? *killTitle : name);
  else {
    if (killTitle)
      return TSentence("CREATURE_TITLE", capitalFirst(bare()), *killTitle);
    else
      return bare();
  }
}

void CreatureName::setFirst(optional<string> s) {
  firstName = s;
}

void CreatureName::generateFirst(NameGenerator* generator) {
  if (firstNameGen)
    firstName = generator->getNext(*firstNameGen);
}

optional<NameGeneratorId> CreatureName::getNameGenerator() const {
  return firstNameGen;
}

void CreatureName::setStack(const TString& s) {
  stackName = s;
}

void CreatureName::setBare(const TString& s) {
  name = s;
}

void CreatureName::modifyName(TStringId id) {
  name = TSentence(std::move(id), std::move(name));
}

const optional<TString>& CreatureName::stackOnly() const {
  return stackName;
}

const TString& CreatureName::stack() const {
  if (stackName)
    return *stackName;
  else
    return name;
}

const optional<string>& CreatureName::first() const {
  return firstName;
}

TString CreatureName::firstOrBare() const {
  if (firstName)
    return TString(*firstName);
  else
    return capitalFirst(bare());
}

void CreatureName::useFullTitle(bool b) {
  fullTitle = b;
}

void CreatureName::setKillTitle(optional<TString> t) {
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
    pluralName = makePlural(name);
}

