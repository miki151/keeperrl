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

const EnumMap<ExperienceType, unordered_set<AttrType, CustomHash<AttrType>>>& getAttrIncreases() {
  static const EnumMap<ExperienceType, unordered_set<AttrType, CustomHash<AttrType>>> attrIncreases {
    {ExperienceType::MELEE, {
        AttrType("DAMAGE"),
        AttrType("DEFENSE")
    }},
    {ExperienceType::SPELL, {
        AttrType("SPELL_DAMAGE"),
    }},
    {ExperienceType::ARCHERY, {
        AttrType("RANGED_DAMAGE")
    }},
  };
  return attrIncreases;
}

optional<ExperienceType> getExperienceType(AttrType attr) {
  static unordered_map<AttrType, optional<ExperienceType>, CustomHash<AttrType>> ret = [] {
    unordered_map<AttrType, optional<ExperienceType>, CustomHash<AttrType>> ret;
    for (auto expType : ENUM_ALL(ExperienceType))
      for (auto attr : getAttrIncreases()[expType])
        ret[attr] = expType;
    return ret;
  }();
  return ret[attr];
}
