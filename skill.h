#ifndef _SKILL_H
#define _SKILL_H

#include <string>

class Skill {
  public:
  string getName() const;
  string getHelpText() const;

  virtual void onTeach(Creature* c) {}

  static Skill* const ambush;
  static Skill* const twoHandedWeapon;
  static Skill* const knifeThrowing;
  static Skill* const stealing;
  static Skill* const mushrooms;
  static Skill* const potions;
  static Skill* const amulets;
  static Skill* const swimming;
  static Skill* const archery;
  static Skill* const construction;

  protected:
  Skill(string name, string helpText);

  private:
  string name;
  string helpText;
};


#endif
