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
#include "inventory.h"
#include "view_index.h"
#include "poison_gas.h"
#include "vision.h"
#include "fire.h"
#include "square_type.h"
#include "renderable.h"
#include "movement_type.h"
#include "view_object.h"

class Level;
class Creature;
class Item;
class CreatureView;
class Attack;
class ProgressMeter;

enum class SquareApplyType { DRINK, USE_CHEST, ASCEND, DESCEND, PRAY, SLEEP, TRAIN, WORKSHOP, TORTURE };

class Square : public Renderable {
  public:
  struct Params {
    string name;
    Vision* vision;
    bool canHide;
    int strength;
    double flamability;
    map<SquareId, int> constructions;
    bool ticking;
    MovementType movementType;
    bool canDestroy;
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
  void setLandingLink(StairDirection direction, StairKey key);

  /** Checks if this square is a point of entry from another level. See setLandingLink().*/
  bool isLandingSquare(StairDirection, StairKey);

  /** Returns the entry point details. Returns none if square is not entry point. See setLandingLink().*/
  optional<pair<StairDirection, StairKey>> getLandingLink() const;

  /** Returns radius of emitted light (0 if none).*/
  virtual double getLightEmission() const;

  /** Sets the height of the square.*/
  void setHeight(double height);

  /** Adds direction for auto-traveling. \paramname{dir} must point to next square on path.*/
  void addTravelDir(Vec2 dir);

  /** Returns auto-travel directions pointing to the next squares on the path.*/
  const vector<Vec2>& getTravelDir() const;

  /** Sets the square's position on the level.*/
  void setPosition(Vec2 v);

  /** Returns the square's position on the level.*/
  Vec2 getPosition() const;

  //@{
  /** Checks if this creature can enter the square at the moment. Takes account other creatures on the square.*/
  bool canEnter(const Creature*) const;
  bool canEnter(MovementType) const;
  //@}

  //@{
  /** Checks if this square is can be entered by the creature. Doesn't take into account other 
    * creatures on the square.*/
  bool canEnterEmpty(const Creature*) const;
  bool canEnterEmpty(MovementType) const;
  //@}

  /** Checks if this square obstructs view.*/
  bool canSeeThru(Vision* = Vision::get(VisionId::NORMAL)) const;

  /** Sets if this square obstructs view.*/
  void setVision(Vision*);

  /** Checks if the player can hide behind this square.*/
  bool canHide() const;

  /** Returns the strength, i.e. resistance to demolition.*/
  int getStrength() const;

  /** Checks if this square can be destroyed.*/
  virtual bool canDestroyBy(const Creature* c) const;
  virtual bool canDestroy() const;

  /** Called when something destroyed this square.*/
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
  void putCreatureSilently(Creature*);

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
  const vector<Trigger*> getTriggers() const;

  /** Removes the trigger from the square.*/
  PTrigger removeTrigger(Trigger*);

  /** Removes all triggers from the square.*/
  vector<PTrigger> removeTriggers();

  //@{
  /** Drops item or items on the square. The square assumes ownership.*/
  virtual void dropItem(PItem);
  void dropItems(vector<PItem>);
  //@}

  /** Checks if a given item is present on the square.*/
  bool hasItem(Item*) const;

  /** Checks if another square can be constructed from this one.*/
  bool canConstruct(SquareType) const;

  /** Constructs another square. The construction might finish after several attempts.
    Returns true if construction was finishd.*/
  bool construct(SquareType);

  /** Called just before swapping the old square for the new constructed one.*/
  virtual void onConstructNewSquare(Square* newSquare) {}
  
  /** Triggers all time-dependent processes like burning. Calls tick() for items if present.
      For this method to be called, the square coordinates must be added with Level::addTickingSquare().*/
  void tick(double time);

  virtual bool canLock() const { return false; }
  virtual bool isLocked() const { FAIL << "BAD"; return false; }
  virtual void lock() { FAIL << "BAD"; }

  optional<ViewObject> getBackgroundObject() const;
  void setBackground(const Square*);
  void getViewIndex(ViewIndex&, const Tribe*) const;

  bool itemLands(vector<Item*> item, const Attack& attack) const;
  virtual bool itemBounces(Item* item, Vision*) const;
  void onItemLands(vector<PItem> item, const Attack& attack, int remainingDist, Vec2 dir, Vision*);
  vector<Item*> getItems(function<bool (Item*)> predicate = alwaysTrue<Item*>()) const;
  PItem removeItem(Item*);
  vector<PItem> removeItems(vector<Item*>);

  virtual optional<SquareApplyType> getApplyType(const Creature*) const { return none; }
  virtual void onApply(Creature* c) { Debug(FATAL) << "Bad square applied"; }
 
  const Level* getConstLevel() const;

  virtual ~Square();

  void setFog(double val);

  bool needsMemoryUpdate() const;
  void setMemoryUpdated();

  Level* getLevel();
  const Level* getLevel() const;

  SERIALIZATION_DECL(Square);

  protected:
  void onEnter(Creature*);
  virtual bool canEnterSpecial(const Creature*) const;
  virtual void onEnterSpecial(Creature*) {}
  virtual void tickSpecial(double time) {}
  Inventory SERIAL(inventory);
  string SERIAL(name);
  const MovementType& getMovementType() const;
  void setMovementType(MovementType);

  private:
  Item* getTopItem() const;

  Level* SERIAL2(level, nullptr);
  Vec2 SERIAL(position);
  Creature* SERIAL2(creature, nullptr);
  vector<PTrigger> SERIAL(triggers);
  optional<ViewObject> SERIAL(backgroundObject);
  Vision* SERIAL(vision);
  bool SERIAL(hide);
  int SERIAL(strength);
  double SERIAL(height);
  vector<Vec2> SERIAL(travelDir);
  optional<pair<StairDirection, StairKey>> SERIAL(landingLink);
  Fire SERIAL(fire);
  PoisonGas SERIAL(poisonGas);
  map<SquareId, int> SERIAL(constructions);
  bool SERIAL(ticking);
  double SERIAL2(fog, 0);
  MovementType SERIAL(movementType);
  bool SERIAL2(updateMemory, true);
  mutable bool SERIAL2(updateViewIndex, true);
  mutable ViewIndex SERIAL(viewIndex);
  void setDirty();
  bool SERIAL2(canDestroySquare, false);
};

#endif
