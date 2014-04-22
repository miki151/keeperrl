#ifndef _ENCYCLOPEDIA_H
#define _ENCYCLOPEDIA_H

#include "collective.h"

class View;
class Technology;
class Deity;

class Encyclopedia {
  public:
  void present(View*, int lastInd = 0);

  private:
  void bestiary(View*, int lastInd = 0);
  void advances(View*, int lastInd = 0);
  void skills(View*, int lastInd = 0);
  void advance(View*, const Technology* tech);
  void rooms(View*, int lastInd = 0);
  void room(View*, Collective::RoomInfo& info);
  void deity(View*, const Deity*);
  void skill(View*, const Skill*);
  void deities(View*, int lastInd = 0);
  void tribes(View*, int lastInd = 0);
  void workshop(View*, int lastInd = 0);
};

#endif
