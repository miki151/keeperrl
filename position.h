#ifndef _POSITION_H
#define _POSITION_H

#include "util.h"

class Square;
class Level;
class PlayerMessage;
class Location;

class Position {
  public:
  Position(Vec2, Level*);
  Vec2 getCoord() const;
  Level* getLevel() const;
  int dist8(const Position&) const;
  bool isSameLevel(const Position&) const;
  Vec2 getDir(const Position&) const;
  Square* getSafeSquare() const;
  Creature* getCreature() const;
  bool operator == (const Position&) const;
  Position& operator = (const Position&);
  Position operator + (Vec2) const;
  Position operator - (Vec2) const;
  bool operator < (const Position&) const;
  void globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const;
  void globalMessage(const PlayerMessage& playerCanSee) const;
  vector<Position> neighbors8(bool shuffle = false) const;
  vector<Position> neighbors4(bool shuffle = false) const;
  void addCreature(PCreature);
  const Location* getLocation() const;

  SERIALIZATION_DECL(Position);

  private:
  Vec2 SERIAL(coord);
  Level* SERIAL(level);
};

namespace std {

template <> struct hash<Position> {
  size_t operator()(const Position& obj) const {
    return hash<Vec2>()(obj.getCoord());
  }
};
}


#endif
