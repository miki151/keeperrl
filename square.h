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
#include "stair_key.h"
#include "tribe.h"

class Creature;
class Item;
class ProgressMeter;
class TileGas;
class Inventory;
class Position;
class ViewIndex;
class Attack;

class Square {
  public:
  Square();

  /** For displaying progress while loading/saving the game.*/
  static ProgressMeter* progressMeter;

  /** Links this square as point of entry from another level.
    * \param direction direction where the creature is coming from
    * \param key id specific to a dungeon branch*/
  void setLandingLink(optional<StairKey>);

  /** Returns the entry point details. Returns none if square is not entry point. See setLandingLink().*/
  optional<StairKey> getLandingLink() const;

  /** Adds some poison gas to the square.*/
  void addGas(Position, TileGasType, double amount);

  /** Returns the amount of poison gas on this square.*/
  double getGasAmount(TileGasType) const;

  /** Sets the level this square is on.*/
  void onAddedToLevel(Position) const;

  /** Puts a creature on the square.*/
  void putCreature(Creature*);

  /** Removes the creature from the square.*/
  void removeCreature(Position);

  /** Returns the creature from the square.*/
  Creature* getCreature() const;

  bool isOnFire() const;
  void setOnFire(bool);

  //@{
  /** Drops item or items on the square. The square assumes ownership.*/
  void dropItem(Position, PItem);
  void dropItems(Position, vector<PItem>);
  void dropItemsLevelGen(vector<PItem>);
  //@}

  /** Triggers all time-dependent processes like burning. Calls tick() for items if present.
      For this method to be called, the square coordinates must be added with Level::addTickingSquare().*/
  void tick(Position);

  void getViewIndex(ViewIndex&, const Creature* viewer) const;

  bool itemLands(vector<Item*> item, const Attack& attack) const;
  void onItemLands(Position, vector<PItem>, const Attack&);
  PItem removeItem(Position, Item*);
  vector<PItem> removeItems(Position, vector<Item*>);

  void forbidMovementForTribe(Position, TribeId);
  void allowMovementForTribe(Position, TribeId);
  bool isTribeForbidden(TribeId) const;
  optional<TribeId> getForbiddenTribe() const;
 
  ~Square();

  bool needsMemoryUpdate() const;
  void setMemoryUpdated();

  void clearItemIndex(ItemIndex);
  void setDirty(Position);

  const Inventory& getInventory() const;

  template <class Archive>
  void serialize(Archive&, const unsigned int);

  private:
  HeapAllocated<Inventory> SERIAL(inventory);
  Creature* SERIAL(creature) = nullptr;
  optional<StairKey> SERIAL(landingLink);
  HeapAllocated<TileGas> SERIAL(tileGas);
  mutable optional<UniqueEntity<Creature>::Id> SERIAL(lastViewer);
  unique_ptr<ViewIndex> SERIAL(viewIndex);
  optional<TribeId> SERIAL(forbiddenTribe);
  bool SERIAL(onFire) = false;
};
