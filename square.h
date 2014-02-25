#ifndef _SQUARE_H
#define _SQUARE_H

#include "util.h"
#include "item.h"
#include "creature.h"
#include "debug.h"
#include "inventory.h"
#include "trigger.h"
#include "view_index.h"
#include "poison_gas.h"

class Level;

class Square {
  public:
  /** Constructs a square objects.
    * \param noObstruct true if the square doesn't obstruct view
    * \param canHide true if the player can hide at this square
    * \param strength square resistance to demolition
    */
  Square(const ViewObject& vo, const string& name, bool noObstruct, bool canHide = false,
      int strength = 0, double flamability = 0, map<SquareType, int> constructions = {}, bool ticking = false);

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

  /** Returns the entry point details. Returns nothing if square is not entry point. See setLandingLink().*/
  Optional<pair<StairDirection, StairKey>> getLandingLink() const;

  /** Marks this square as covered, i.e. having a roof.*/
  void setCovered(bool);

  /** Checks if this square is covered, i.e. having a roof.*/
  bool isCovered() const;

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

  /** Checks if this creature can enter the square at the moment. Takes account other creatures on the square.*/
  bool canEnter(const Creature*) const;

  /** Checks if this square is can be entered by the creature. Doesn't take into account other 
    * creatures on the square.*/
  bool canEnterEmpty(const Creature*) const;

  /** Checks if this square obstructs view.*/
  bool canSeeThru() const;

  /** Sets if this square obstructs view.*/
  void setCanSeeThru(bool);

  /** Checks if the player can hide behind this square.*/
  bool canHide() const;

  /** Returns the strength, i.e. resistance to demolition.*/
  int getStrength() const;

  /** Checks if this square can be destroyed.*/
  virtual bool canDestroy() const { return false; }

  /** Called when something destroyed this square.*/
  virtual void destroy(int strength);

  /** Called when this square is burned completely.*/
  virtual void burnOut();

  /** Exposes the square and objects on it to fire.*/
  void setOnFire(double amount);

  bool isBurnt() const;

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

  /** Removes the trigger from the square.*/
  PTrigger removeTrigger(Trigger*);

  /** Removes all triggers from the square.*/
  void removeTriggers();

  //@{
  /** Drops item or items on the square. The square assumes ownership.*/
  virtual void dropItem(PItem);
  void dropItems(vector<PItem>);
  //@}

  /** Checks if a given item is present on the square.*/
  bool hasItem(Item*) const;

  /** Checks if another square can be constructed from this one.*/
  bool canConstruct(SquareType) const;

  /** Constructs another square. The construction might finish after several attempts.*/
  void construct(SquareType);

  /** Called just before swapping the old square for the new constructed one.*/
  virtual void onConstructNewSquare(Square* newSquare) {}
  
  /** Triggers all time-dependent processes like burning. Calls tick() for creature and items if present.
      For this method to be called, the square coordinates must be added with Level::addTickingSquare().*/
  void tick(double time);

  ViewObject getViewObject() const;
  Optional<ViewObject> getBackgroundObject() const;
  void setBackground(const Square*);
  ViewIndex getViewIndex(const CreatureView* c) const;

  bool itemLands(Item* item, const Attack& attack);
  virtual bool itemBounces(Item* item) const;
  void onItemLands(PItem item, const Attack& attack, int remainingDist, Vec2 dir);
  vector<Item*> getItems(function<bool (Item*)> predicate = alwaysTrue<Item*>());
  PItem removeItem(Item*);
  vector<PItem> removeItems(vector<Item*>);

  virtual Optional<SquareApplyType> getApplyType(const Creature*) const { return Nothing(); }
  virtual void onApply(Creature* c) { Debug(FATAL) << "Bad square applied"; }
 
  const Level* getConstLevel() const;

  virtual ~Square() {};

  void setFog(double val);

  protected:
  void onEnter(Creature*);
  virtual bool canEnterSpecial(const Creature*) const;
  virtual void onEnterSpecial(Creature*) {}
  virtual void tickSpecial(double time) {}
  Level* getLevel();
  void setViewObject(const ViewObject&);
  vector<Vec2> face;
  Inventory inventory;
  string name;

  private:
  Item* getTopItem() const;

  Level* level = nullptr;
  Vec2 position;
  Creature* creature = nullptr;
  vector<PTrigger> triggers;
  ViewObject viewObject;
  Optional<ViewObject> backgroundObject;
  bool seeThru;
  bool hide;
  int strength;
  double height;
  vector<Vec2> travelDir;
  bool covered = false;
  Optional<pair<StairDirection, StairKey>> landingLink;
  Fire fire;
  PoisonGas poisonGas;
  map<SquareType, int> constructions;
  bool ticking;
  double fog = 0;
};

class SolidSquare : public Square {
  public:
  SolidSquare(const ViewObject& vo, const string& name, bool canSee, map<SquareType, int> constructions = {},
      bool alwaysVisible = false, double flamability = 0) :
      Square(vo, name, canSee, false, 0, flamability, constructions) {
  }

  protected:
  virtual bool canEnterSpecial(const Creature*) const;

  virtual void onEnterSpecial(Creature* c) {
    Debug(FATAL) << "Creature entered solid square";
  }
};

#endif
