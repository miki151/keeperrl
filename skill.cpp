#include "stdafx.h"

#include "skill.h"
#include "item_factory.h"


template <class Archive> 
void Skill::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(helpText);
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
    ar & SUBCLASS(Skill) & BOOST_SERIALIZATION_NVP(factory);
  }

  SERIALIZATION_CONSTRUCTOR(IdentifySkill);
  private:
  ItemFactory factory;
};

template <class Archive>
void Skill::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, IdentifySkill);
}

REGISTER_TYPES(Skill);

Skill* Skill::ambush = new Skill("skill of ambush" ,"Press \'h\' to hide and attack unsuspecting enemies.");
Skill* Skill::twoHandedWeapon = new Skill("skill of fighting with two-handed weapons", 
      "You can now fight with warhammers and battle axes.");
Skill* Skill::knifeThrowing = new Skill("skill of knife throwing",
    "You can now throw knives with deadly precision.");
Skill* Skill::stealing = new Skill("skill of stealing", "Not available");
Skill* Skill::mushrooms = new IdentifySkill("knowledge of mushrooms",
    "You now know the types of mushrooms and their use.", ItemFactory::mushrooms());
Skill* Skill::potions = new IdentifySkill("knowledge of potions",
    "You now know the types of potions and their use.", ItemFactory::potions());
Skill* Skill::amulets = new IdentifySkill("knowledge of amulets",
    "You now know the types of amulets and their use.", ItemFactory::amulets());
Skill* Skill::swimming = new Skill("skill of swimming", "");
Skill* Skill::archery = new Skill("skill of archery",
    "You can now equip a bow and use \'alt + arrow\' to fire");
Skill* Skill::construction = new Skill("skill of construction", "");

Skill::Skill(string _name, string _helpText) : name(_name), helpText(_helpText) {}

