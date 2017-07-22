#include "stdafx.h"
#include "experience_type.h"
#include "attr_type.h"

const char* getName(ExperienceType type) {
  switch (type) {
    case ExperienceType::SPELL:
      return "Spell";
    case ExperienceType::MELEE:
      return "Melee";
    case ExperienceType::ARCHERY:
      return "Archery";
  }
}

const static EnumMap<ExperienceType, string> lowerCaseName([](ExperienceType type) { return toLower(getName(type)); });

const char* getNameLowerCase(ExperienceType type) {
  return lowerCaseName[type].c_str();
}

static const EnumMap<ExperienceType, EnumSet<AttrType>> attrIncreases {
  {ExperienceType::MELEE, {
      AttrType::DAMAGE,
      AttrType::DEFENSE
  }},
  {ExperienceType::SPELL, {
      AttrType::SPELL_DAMAGE,
  }},
  {ExperienceType::ARCHERY, {
      AttrType::RANGED_DAMAGE
  }},
};

const EnumMap<ExperienceType, EnumSet<AttrType>>& getAttrIncreases() {
  return attrIncreases;
}

optional<ExperienceType> getExperienceType(AttrType attr) {
  static EnumMap<AttrType, optional<ExperienceType>> ret(
      [](AttrType attr) -> optional<ExperienceType> {
        for (auto expType : ENUM_ALL(ExperienceType))
          if (attrIncreases[expType].contains(attr))
            return expType;
        return none;
      });
  return ret[attr];
}
