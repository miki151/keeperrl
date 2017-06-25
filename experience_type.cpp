#include "stdafx.h"
#include "experience_type.h"

const char* getName(ExperienceType type) {
  switch (type) {
    case ExperienceType::STUDY:
      return "Mage level";
    case ExperienceType::TRAINING:
      return "Warrior level";
  }
}

char getSymbol(ExperienceType type) {
  switch (type) {
    case ExperienceType::STUDY:
      return 'M';
    case ExperienceType::TRAINING:
      return 'W';
  }
}

const char* getNameLowerCase(ExperienceType type) {
  switch (type) {
    case ExperienceType::STUDY:
      return "mage level";
    case ExperienceType::TRAINING:
      return "warrior level";
  }
}
