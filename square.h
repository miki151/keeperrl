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

#pragma once

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
    HeapAllocated<MovementSet> movementSet;
    bool canDestroy;
    optional<SoundId> applySound;
    optional<double> applyTime;
    optional<SquareInteraction> interaction;
  };
  Square(const ViewObject&, Params);

  /** For displaying progress while loading/saving the game.*/
  static ProgressMeter* progressMeter;

  /** Returns the square name. */
  string getName() const;

  /** Sets the square name.*/
  void setName(Position, const string&);

  /** Links this square as point of entry from another level.
    * \param direction direction where the creature is coming from
    * \param key id specific to a dungeon branch*/
  void setLandingLink(StairKey);

  /** Returns the entry point details. Returns none if square is not entry point. See setLandingLink().*/
  optional<StairKey> getLandingLink() const;

  /** Checks if this square obstructs view.*/
  bool canSeeThru(VisionId) const;
  bool canSeeThru() const;

  /** Sets if this square obstructs view.*/
  void setVision(Position, VisionId);

  /** Returns the strength, i.e. resistance to demolition.*/
  int getStrength() const;

  /** Adds some poison gas to the square.*/
  void addPoisonGas(Position, double amount);

  /** Returns the amount of poison gas on this square.*/
  double getPoisonGasAmount() const;

  /** Sets the level this square is on.*/
  void onAddedToLevel(Position) const;

  /** Puts a creature on the square.*/
  void putCreature(Creature*);

  /** Puts a creature on the square without triggering any mechanisms that happen when a creature enters.*/ 
  void setCreature(Creature*);

  /** Removes the creature from the square.*/
  void removeCreature(Position);

  /** Returns the creature from the square.*/
  Creature* getCreature() const;

  /** Adds a trigger to the square.*/
  void addTrigger(Position, PTrigger);

  /** Returns all triggers.*/
  vector<Trigger*> getTriggers() const;

  /** Removes the trigger from the square.*/
  PTrigger removeTrigger(Position, Trigger*);

  /** Removes all triggers from the square.*/
  vector<PTrigger> removeTriggers(Position);

  //@{
  /** Drops item or items on the square. The square assumes ownership.*/
  void dropItem(Position, PItem);
  virtual void dropItems(Position, vector<PItem>);
  void dropItemsLevelGen(vector<PItem>);
  //@}

  /** Triggers all time-dependent processes like burning. Calls tick() for items if present.
      For this method to be called, the square coordinates must be added with Level::addTickingSquare().*/
  void tick(Position);

  optional<ViewObject> extractBackground() const;
  void getViewIndex(ViewIndex&, const Creature* viewer) const;

  bool itemLands(vector<Item*> item, const Attack& attack) const;
  void onItemLands(Position, vector<PItem>, const Attack&, int remainingDist, Vec2 dir, VisionId);
  const vector<Item*>& getItems() const;
  vector<Item*> getItems(function<bool (Item*)> predicate) const;
  const vector<Item*>& getItems(ItemIndex) const;
  PItem removeItem(Position, Item*);
  vector<PItem> removeItems(Position, vector<Item*>);

  void forbidMovementForTribe(Position, TribeId);
  void allowMovementForTribe(Position, TribeId);
  bool isTribeForbidden(TribeId) const;
  optional<TribeId> getForbiddenTribe() const;
 
  virtual ~Square();

  bool needsMemoryUpdate() const;
  void setMemoryUpdated();

  void clearItemIndex(ItemIndex);
  void setDirty(Position);
  MovementSet& getMovementSet();
  const MovementSet& getMovementSet() const;

  Inventory& getInventory();
  const Inventory& getInventory() const;

  SERIALIZATION_DECL(Square);

  protected:
  void onEnter(Creature*);
  virtual void onEnterSpecial(Creature*) {}
  virtual void onApply(Creature*) { FATAL << "Bad square applied"; }
  virtual void onApply(Position) { FATAL << "Bad square applied"; }
  string SERIAL(name);

  private:
  Item* getTopItem() const;
  HeapAllocated<Inventory> SERIAL(inventory);
  Creature* SERIAL(creature) = nullptr;
  vector<PTrigger> SERIAL(triggers);
  optional<VisionId> SERIAL(vision);
  optional<StairKey> SERIAL(landingLink);
  HeapAllocated<PoisonGas> SERIAL(poisonGas);
  HeapAllocated<MovementSet> SERIAL(movementSet);
  mutable optional<UniqueEntity<Creature>::Id> SERIAL(lastViewer);
  unique_ptr<ViewIndex> SERIAL(viewIndex);
  optional<TribeId> SERIAL(forbiddenTribe);
};
