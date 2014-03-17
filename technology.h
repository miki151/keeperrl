#ifndef _TECHNOLOGY_H
#define _TECHNOLOGY_H

#include "singleton.h"
#include "enums.h"

class Technology : public Singleton<Technology, TechId> {
  public:
  Technology(const string& name, int cost, const vector<TechId>& prerequisites, bool canResearch = true);
  const string& getName() const;
  int getCost() const;
  bool canResearch() const;
  static vector<Technology*> getSorted();
  const vector<Technology*> getPrerequisites() const;
  const vector<Technology*> getAllowed() const;

  static vector<Technology*> getNextTechs(const vector<Technology*>& current);

  static void init();

  SERIALIZATION_DECL(Technology);

  private:
  bool canLearnFrom(const vector<Technology*>&) const;
  string name;
  int cost;
  vector<Technology*> prerequisites;
  bool research;
};

#endif
