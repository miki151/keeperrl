#include "stdafx.h"

#include "skill.h"
#include "item_factory.h"
#include "enums.h"

template <class Archive> 
void Skill::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(name)
    & SVAR(helpText);
  CHECK_SERIAL;
}

SERIALIZABLE(Skill);

string Skill::getName() const {
  return name;
}

string Skill::getHelpText() const {
  return helpText;
}

class IdentifySkill : public Skill {
  public:
  IdentifySkill(string name, string helpText, ItemFactory f) : Skill(name, helpText), factory(f) {}

  virtual void onTeach(Creature* c) override {
    string message;
    for (PItem& it : factory.getAll()) {
      Item::identify(it->getName());
      message.append(it->getName() + ": " + it->getDescription() + "\n");
    }
 //   c->privateMessage(MessageBuffer::important(message));
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Skill) & SVAR(factory);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(IdentifySkill);
  private:
  ItemFactory SERIAL(factory);
};

template <class Archive>
void Skill::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, IdentifySkill);
}

REGISTER_TYPES(Skill);

void Skill::init() {
  Skill::set(SkillId::AMBUSH, new Skill("ambush",
        "Hide and ambush unsuspecting enemies. Press 'h' to hide on a tile that allows it."));
  Skill::set(SkillId::TWO_HANDED_WEAPON, new Skill("two-handed weapons", "Fight with "
        "warhammers and battle axes."));
  Skill::set(SkillId::KNIFE_THROWING, new Skill("knife throwing", "Throw knives with deadly precision."));
  Skill::set(SkillId::STEALING, new Skill("stealing", "Steal from other monsters. Not available for player ATM."));
  Skill::set(SkillId::SWIMMING, new Skill("swimming", "Cross water without drowning."));
  Skill::set(SkillId::ARCHERY, new Skill("archery", "Shoot bows."));
  Skill::set(SkillId::CONSTRUCTION, new Skill("construction", "Mine and construct rooms."));
}

Skill::Skill(string _name, string _helpText) : name(_name), helpText(_helpText) {}

