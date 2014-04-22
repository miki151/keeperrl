#ifndef _SKILL_H
#define _SKILL_H

#include <string>

#include "singleton.h"
#include "enums.h"

class Creature;
class Skill : public Singleton<Skill, SkillId> {
  public:
  string getName() const;
  string getHelpText() const;

  virtual void onTeach(Creature* c) {}

  SERIALIZATION_DECL(Skill);
  
  static void init();

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  Skill(string name, string helpText);

  private:
  string SERIAL(name);
  string SERIAL(helpText);
};


#endif
