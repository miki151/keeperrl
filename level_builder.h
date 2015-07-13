#ifndef _LEVEL_BUILDER_H
#define _LEVEL_BUILDER_H


#include "util.h"
#include "square_type.h"
#include "level.h"

class ProgressMeter;
class Model;
class LevelMaker;
class Square;

class LevelBuilder {
  public:
  /** Constructs a builder with given size and name. */
  LevelBuilder(ProgressMeter&, int width, int height, const string& name, bool covered = true);
  
  /** Move constructor.*/
  LevelBuilder(LevelBuilder&&) = default;

  /** Returns a given square.*/
  Square* getSquare(Vec2);

  /** Checks if it's possible to put a creature on given square.*/
  bool canPutCreature(Vec2, Creature*);

  /** Puts a creatue on a given square. If the square is later changed to something else, the creature remains.*/
  void putCreature(Vec2, PCreature);

  /** Puts items on a given square. If the square is later changed to something else, the items remain.*/
  void putItems(Vec2, vector<PItem> items);

  /** Sets the message displayed when the player first enters the level.*/
  void setMessage(const string& message);

  /** Builds the level. The level will keep reference to the model.
      \paramname{surface} tells if this level is on the Earth surface.*/
  PLevel build(Model*, LevelMaker*, int levelId);

  //@{
  /** Puts a square on given position. Sets optional attributes of the square. The attributes remain if the square is changed.*/
  void putSquare(Vec2, SquareType, optional<SquareAttrib> = none);
  void putSquare(Vec2, SquareType, vector<SquareAttrib> attribs);
  void putSquare(Vec2, PSquare, SquareType, optional<SquareAttrib> = none);
  void putSquare(Vec2, PSquare, SquareType, vector<SquareAttrib> attribs);
  //@}

  /** Returns the square type.*/
  const SquareType& getType(Vec2);

  /** Checks if the given square has an attribute.*/
  bool hasAttrib(Vec2 pos, SquareAttrib attr);

  /** Adds attribute to given square. The attribute will remain if the square is changed.*/
  void addAttrib(Vec2 pos, SquareAttrib attr);

  /** Removes attribute from given square.*/
  void removeAttrib(Vec2 pos, SquareAttrib attr);

  /** Sets the height of the given square.*/
  void setHeightMap(Vec2 pos, double h);

  /** Returns the height of the given square.*/
  double getHeightMap(Vec2 pos);

  /** Adds a location to the level and sets its coordinates.*/
  void addLocation(Location*, Rectangle area);

  Rectangle toGlobalCoordinates(Rectangle);

  /** Adds a collective to the level and initializes it.*/
  void addCollective(CollectiveBuilder*);

  /** Sets the cover of the square. The value will remain if square is changed.*/
  void setCoverInfo(Vec2, Level::CoverInfo);
 
  enum Rot { CW0, CW1, CW2, CW3};

  void pushMap(Rectangle bounds, Rot);
  void popMap();
  
  private:
  bool isInSunlight(Vec2);
  Vec2 transform(Vec2);
  Table<PSquare> squares;
  Table<double> heightMap;
  Table<double> dark;
  vector<Location*> locations;
  vector<CollectiveBuilder*> collectives;
  Table<Level::CoverInfo> coverInfo;
  Table<EnumSet<SquareAttrib>> attrib;
  Table<SquareType> type;
  vector<PCreature> creatures;
  Table<vector<PItem>> items;
  string entryMessage;
  string name;
  vector<Vec2::LinearMap> mapStack;
  ProgressMeter& progressMeter;
};

#endif
