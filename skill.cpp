#include "stdafx.h"

#include "skill.h"
#include "item_factory.h"

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

  private:
  ItemFactory factory;
};

Skill* const Skill::ambush = new Skill("skill of ambush" ,"Press \'h\' to hide and attack unsuspecting enemies.");
Skill* const Skill::twoHandedWeapon = new Skill("skill of fighting with two-handed weapons", 
      "You can now fight with warhammers and battle axes.");
Skill* const Skill::knifeThrowing = new Skill("skill of knife throwing",
    "You can now throw knives with deadly precision.");
Skill* const Skill::stealing = new Skill("skill of stealing", "Not available");
Skill* const Skill::mushrooms = new IdentifySkill("knowledge of mushrooms",
    "You now know the types of mushrooms and their use.", ItemFactory::mushrooms());
Skill* const Skill::potions = new IdentifySkill("knowledge of potions",
    "You now know the types of potions and their use.", ItemFactory::potions());
Skill* const Skill::amulets = new IdentifySkill("knowledge of amulets",
    "You now know the types of amulets and their use.", ItemFactory::amulets());
Skill* const Skill::swimming = new Skill("skill of swimming", "");
Skill* const Skill::archery = new Skill("skill of archery",
    "You can now equip a bow and use \'alt + arrow\' to fire");
Skill* const Skill::construction = new Skill("skill of construction", "");

Skill::Skill(string _name, string _helpText) : name(_name), helpText(_helpText) {}

