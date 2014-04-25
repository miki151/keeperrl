#ifndef _SECTORS_H
#define _SECTORS_H

#include "util.h"

class Sectors {
  public:
  Sectors(Rectangle bounds);

  bool same(Vec2, Vec2) const;
  void add(Vec2);
  void remove(Vec2);

  SERIALIZATION_DECL(Sectors);

  private:
  void setSector(Vec2, int);
  int getNewSector();
  void join(Vec2, int);
  Rectangle SERIAL(bounds);
  Table<int> SERIAL(sectors);
  vector<int> SERIAL(sizes);
};

#endif
