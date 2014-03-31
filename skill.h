#ifndef _SKILL_H
#define _SKILL_H

#include <string>

class Creature;
class Skill {
  public:
  string getName() const;
  string getHelpText() const;

  virtual void onTeach(Creature* c) {}

  static Skill* ambush;
  static Skill* twoHandedWeapon;
  static Skill* knifeThrowing;
  static Skill* stealing;
  static Skill* mushrooms;
  static Skill* potions;
  static Skill* amulets;
  static Skill* swimming;
  static Skill* archery;
  static Skill* construction;

  SERIALIZATION_DECL(Skill);
  
  template <class Archive>
  static void registerTypes(Archive& ar);

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar& BOOST_SERIALIZATION_NVP(ambush)
      & BOOST_SERIALIZATION_NVP(twoHandedWeapon)
      & BOOST_SERIALIZATION_NVP(knifeThrowing)
      & BOOST_SERIALIZATION_NVP(stealing)
      & BOOST_SERIALIZATION_NVP(mushrooms)
      & BOOST_SERIALIZATION_NVP(potions)
      & BOOST_SERIALIZATION_NVP(amulets)
      & BOOST_SERIALIZATION_NVP(swimming)
      & BOOST_SERIALIZATION_NVP(archery)
      & BOOST_SERIALIZATION_NVP(construction);
  }

  protected:
  Skill(string name, string helpText);

  private:
  string SERIAL(name);
  string SERIAL(helpText);
};


#endif
