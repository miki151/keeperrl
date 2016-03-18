#ifndef _POSITION_H
#define _POSITION_H

#include "util.h"

class Square;
class Level;
class PlayerMessage;
class Location;
class Tribe;
class ViewIndex;
class SquareType;
class MovementType;
struct CoverInfo;
class Attack;
class Game;

class Position {
  public:
  Position(Vec2, Level*);
  static vector<Position> getAll(Level*, Rectangle);
  Model* getModel() const;
  Game* getGame() const;
  int dist8(const Position&) const;
  bool isSameLevel(const Position&) const;
  bool isSameLevel(const Level*) const;
  bool isSameModel(const Position&) const;
  Vec2 getDir(const Position&) const;
  Creature* getCreature() const;
  void removeCreature();
  void putCreature(Creature*);
  string getName() const;
  Position withCoord(Vec2 newCoord) const;
  Vec2 getCoord() const;
  Level* getLevel() const;
 
  bool isValid() const;
  bool operator == (const Position&) const;
  bool operator != (const Position&) const;
  Position& operator = (const Position&);
  Position plus(Vec2) const;
  Position minus(Vec2) const;
  bool operator < (const Position&) const;
  void globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const;
  void globalMessage(const PlayerMessage& playerCanSee) const;
  void globalMessage(const Creature*, const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const;
  vector<Position> neighbors8() const;
  vector<Position> neighbors4() const;
  vector<Position> neighbors8(RandomGen&) const;
  vector<Position> neighbors4(RandomGen&) const;
  vector<Position> getRectangle(Rectangle) const;
  void addCreature(PCreature, double delay = 0);
  const Location* getLocation() const;
  bool canEnter(const Creature*) const;
  bool canEnter(const MovementType&) const;
  bool canEnterEmpty(const Creature*) const;
  bool canEnterEmpty(const MovementType&) const;
  optional<SquareApplyType> getApplyType() const;
  optional<SquareApplyType> getApplyType(const Creature*) const;
  void onApply(Creature*);
  double getApplyTime() const;
  bool canHide() const;
  void getViewIndex(ViewIndex&, TribeId) const;
  vector<Trigger*> getTriggers() const;
  PTrigger removeTrigger(Trigger*);
  vector<PTrigger> removeTriggers();
  void addTrigger(PTrigger);
  const vector<Item*>& getItems() const;
  vector<Item*> getItems(function<bool (Item*)> predicate) const;
  const vector<Item*>& getItems(ItemIndex) const;
  PItem removeItem(Item*);
  vector<PItem> removeItems(vector<Item*>);
  bool canConstruct(const SquareType&) const;
  bool canDestroy(const Creature*) const;
  bool isDestroyable() const;
  void dropItem(PItem);
  void dropItems(vector<PItem>);
  void destroyBy(Creature* c);
  void destroy();
  bool construct(const SquareType&);
  bool canLock() const;
  bool isLocked() const;
  void lock();
  bool isBurning() const;
  void setOnFire(double amount);
  bool needsMemoryUpdate() const;
  void setMemoryUpdated();
  const ViewObject& getViewObject() const;
  void forbidMovementForTribe(TribeId);
  void allowMovementForTribe(TribeId);
  bool isTribeForbidden(TribeId) const;
  optional<TribeId> getForbiddenTribe() const;
  void addPoisonGas(double amount);
  double getPoisonGasAmount() const;
  CoverInfo getCoverInfo() const;
  bool sunlightBurns() const;
  void throwItem(PItem item, const Attack& attack, int maxDist, Vec2 direction, VisionId);
  void throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 direction, VisionId);
  bool canNavigate(const MovementType&) const;
  vector<Position> getVisibleTiles(VisionId);
  const vector<Vec2>& getTravelDir() const;
  int getStrength() const;
  bool canSeeThru(VisionId) const;
  bool isVisibleBy(const Creature*);
  void clearItemIndex(ItemIndex);
  bool isChokePoint(const MovementType&) const;
  bool isConnectedTo(Position, const MovementType&) const;
  vector<Creature*> getAllCreatures(int range) const;
  void moveCreature(Vec2 direction);
  void moveCreature(Position);
  bool canMoveCreature(Vec2 direction) const;
  bool canMoveCreature(Position) const;
  void swapCreatures(Creature*);
  double getLight() const;
  optional<Position> getStairsTo(Position) const;

  SERIALIZATION_DECL(Position);
  int getHash() const;

  private:
  Square* getSquare() const;
  Vec2 SERIAL(coord);
  Level* SERIAL(level) = nullptr;
};


#endif
