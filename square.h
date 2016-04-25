/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _SQUARE_H
#define _SQUARE_H

#include "util.h"
#include "debug.h"
#include "renderable.h"
#include "stair_key.h"
#include "position.h"
#include "tribe.h"

class Level;
class Creature;
class Item;
class CreatureView;
class Attack;
class ProgressMeter;
class SquareType;
class ViewIndex;
class Fire;
class PoisonGas;
class Inventory;
class MovementType;
class MovementSet;
class ViewObject;

class Square : public Renderable {
  public:
  struct Params {
    string name;
    optional<VisionId> vision;
    bool canHide;
    int strength;
    double flamability;
    map<SquareId, int> constructions;
    bool ticking;
    HeapAllocated<MovementSet> movementSet;
    bool canDestroy;
    optional<TribeId> owner;
    optional<SoundId> applySound;
  };
  Square(const ViewObject&, Params);

  /** For displaying progress while loading/saving the game.*/
  static ProgressMeter* progressMeter;

  /** Returns the square name. */
  string getName() const;

  /** Sets the square name.*/
  void setName(const string&);

  /** Links this square as point of entry from another level.
    * \param direction direction where the creature is coming from
    * \param key id specific to a dungeon branch*/
  void setLandingLink(StairKey);

  /** Returns the entry point details. Returns none if square is not entry point. See setLandingLink().*/
  optional<StairKey> getLandingLink() const;

  /** Returns radius of emitted light (0 if none).*/
  virtual double getLightEmission() const;

  /** Adds direction for auto-traveling. \paramname{dir} must point to next square on path.*/
  void addTravelDir(Vec2 dir);

  /** Returns auto-travel directions pointing to the next squares on the path.*/
  const vector<Vec2>& getTravelDir() const;

  /** Sets the square's position on the level.*/
  void setPosition(Vec2 v);

  /** Returns the square's position on the level.*/
  Vec2 getPosition() const;
  Position getPosition2() const;

  //@{
  /** Checks if this creature can enter the square at the moment. Takes account other creatures on the square.*/
  bool canEnter(const Creature*) const;
  bool canEnter(const MovementType&) const;
  //@}

  bool canNavigate(const MovementType&) const;

  //@{
  /** Checks if this square is can be entered by the creature. Doesn't take into account other 
    * creatures on the square.*/
  bool canEnterEmpty(const Creature*) const;
  bool canEnterEmpty(const MovementType&) const;
  //@}

  /** Checks if this square obstructs view.*/
  bool canSeeThru(VisionId) const;
  bool canSeeThru() const;

  /** Sets if this square obstructs view.*/
  void setVision(VisionId);

  /** Checks if the player can hide behind this square.*/
  bool canHide() const;

  /** Returns the strength, i.e. resistance to demolition.*/
  int getStrength() const;

  /** Checks if this square can be destroyed using the 'destroy' order.*/
  bool isDestroyable() const;

  /** Checks if this square can be destroyed by a creature. Pathfinding will not take into account this result.*/
  bool canDestroy(const Creature*) const;

  /** Called when something is destroying this square (may take a few turns to destroy).*/
  virtual void destroyBy(Creature* c);
  virtual void destroy();

  /** Called when this square is burned completely.*/
  virtual void burnOut();

  /** Exposes the square and objects on it to fire.*/
  void setOnFire(double amount);

  /** Returns whether the square is currently on fire.*/
  bool isBurning() const;

  /** Adds some poison gas to the square.*/
  void addPoisonGas(double amount);

  /** Returns the amount of poison gas on this square.*/
  double getPoisonGasAmount() const;

  /** Sets the level this square is on.*/
  void setLevel(Level*);

  /** Puts a creature on the square.*/
  void putCreature(Creature*);

  /** Puts a creature on the square without triggering any mechanisms that happen when a creature enters.*/ 
  void setCreature(Creature*);

  /** Removes the creature from the square.*/
  void removeCreature();

  //@{
  /** Returns the creature from the square.*/
  Creature* getCreature();
  const Creature* getCreature() const;
  //@}

  /** Adds a trigger to the square.*/
  void addTrigger(PTrigger);

  /** Returns all triggers.*/
  vector<Trigger*> getTriggers() const;

  /** Removes the trigger from the square.*/
  PTrigger removeTrigger(Trigger*);

  /** Removes all triggers from the square.*/
  vector<PTrigger> removeTriggers();

  //@{
  /** Drops item or items on the square. The square assumes ownership.*/
  void dropItem(PItem);
  virtual void dropItems(vector<PItem>);
  //@}

  /** Checks if a given item is present on the square.*/
  bool hasItem(Item*) const;

  /** Checks if another square can be constructed from this one.*/
  bool canConstruct(const SquareType&) const;

  /** Constructs another square. The construction might finish after several attempts.
    Returns true if construction was finishd.*/
  bool construct(const SquareType&);

  /** Called just before swapping the old square for the new constructed one.*/
  virtual void onConstructNewSquare(Square* newSquare) {}
  
  /** Triggers all time-dependent processes like burning. Calls tick() for items if present.
      For this method to be called, the square coordinates must be added with Level::addTickingSquare().*/
  void tick();
  void updateSunlightMovement(bool isSunlight);

  virtual bool canLock() const { return false; }
  virtual bool isLocked() const { FAIL << "BAD"; return false; }
  virtual void lock() { FAIL << "BAD"; }

  bool sunlightBurns() const;

  void setBackground(const Square*);
  void getViewIndex(ViewIndex&, TribeId) const;

  bool itemLands(vector<Item*> item, const Attack& attack) const;
  bool itemBounces(Item* item, VisionId) const;
  void onItemLands(vector<PItem> item, const Attack& attack, int remainingDist, Vec2 dir, VisionId);
  const vector<Item*>& getItems() const;
  vector<Item*> getItems(function<bool (Item*)> predicate) const;
  const vector<Item*>& getItems(ItemIndex) const;
  PItem removeItem(Item*);
  vector<PItem> removeItems(vector<Item*>);

  virtual optional<SquareApplyType> getApplyType() const { return none; }
  virtual bool canApply(const Creature*) const { return true; }
  void apply(Creature*);
  virtual double getApplyTime() const { return 1.0; }
  optional<SquareApplyType> getApplyType(const Creature*) const;

  void forbidMovementForTribe(TribeId);
  void allowMovementForTribe(TribeId);
  bool isTribeForbidden(TribeId) const;
  optional<TribeId> getForbiddenTribe() const;
 
  virtual ~Square();

  void setUnavailable();
  bool isUnavailable() const;

  bool needsMemoryUpdate() const;
  void setMemoryUpdated();

  Level* getLevel();
  const Level* getLevel() const;
  void clearItemIndex(ItemIndex);

  SERIALIZATION_DECL(Square);

  protected:
  void onEnter(Creature*);
  virtual void onEnterSpecial(Creature*) {}
  virtual void tickSpecial() {}
  virtual void onApply(Creature*) { Debug(FATAL) << "Bad square applied"; }
  string SERIAL(name);
  void addTraitForTribe(TribeId, MovementTrait);
  void removeTraitForTribe(TribeId, MovementTrait);
  void setDirty();

  Inventory& getInventory();
  const Inventory& getInventory() const;
  bool inventoryEmpty() const;

  private:
  Item* getTopItem() const;
  mutable unique_ptr<Inventory> SERIAL(inventoryPtr);

  /** Checks if this square can be destroyed by member of the tribe.*/
  bool canDestroy(TribeId) const;

  Level* SERIAL(level) = nullptr;
  Vec2 SERIAL(position);
  Creature* SERIAL(creature) = nullptr;
  vector<PTrigger> SERIAL(triggers);
  PViewObject SERIAL(backgroundObject);
  optional<VisionId> SERIAL(vision);
  bool SERIAL(hide);
  int SERIAL(strength);
  vector<Vec2> SERIAL(travelDir);
  optional<StairKey> SERIAL(landingLink);
  HeapAllocated<Fire> SERIAL(fire);
  HeapAllocated<PoisonGas> SERIAL(poisonGas);
  map<SquareId, int> SERIAL(constructions);
  bool SERIAL(ticking);
  HeapAllocated<MovementSet> SERIAL(movementSet);
  void updateMovement();
  bool SERIAL(updateMemory) = true;
  mutable bool SERIAL(updateViewIndex) = true;
  unique_ptr<ViewIndex> SERIAL(viewIndex);
  bool SERIAL(destroyable) = false;
  optional<TribeId> SERIAL(owner);
  optional<TribeId> SERIAL(forbiddenTribe);
  bool SERIAL(unavailable) = false;
  optional<SoundId> SERIAL(applySound);
};

#endif
