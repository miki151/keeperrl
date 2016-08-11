#pragma once

#include "position.h"
#include "renderable.h"

class Creature;
class MovementType;

class Furniture : public Renderable {
  public:
  enum Type { BLOCKING, NON_BLOCKING };
  Furniture(const string& name, const ViewObject&, Type);
  void apply(Position, Creature*);
  double getApplyTime() const;
  const string& getName() const;
  bool isWalkable() const;
  void onDestroy(Position);

  SERIALIZATION_DECL(Furniture);

  private:
  string SERIAL(name);
  Type SERIAL(type);
};
