#pragma once

#include "util.h"
#include "level.h"
#include "square_array.h"
#include "furniture_array.h"
#include "view_object.h"

class ProgressMeter;
class Model;
class LevelMaker;
class Square;
class FurnitureFactory;
class CollectiveBuilder;

RICH_ENUM(SquareAttrib,
  NO_DIG,
  MOUNTAIN,
  HILL,
  LOWLAND,
  CONNECT_ROAD, 
  CONNECT_CORRIDOR,
  CONNECTOR,
  LAKE,
  RIVER,
  ROAD_CUT_THRU,
  NO_ROAD,
  ROOM,
  ROOM_WALL,
  COLLECTIVE_START,
  COLLECTIVE_STAIRS,
  EMPTY_ROOM,
  BUILDINGS_CENTER,
  CASTLE_CORNER,
  FOG,
  FORREST,
  SOKOBAN_ENTRY,
  SOKOBAN_PRIZE
);

class LevelBuilder {
  public:
  /** Constructs a builder with given size and name. */
  LevelBuilder(ProgressMeter*, RandomGen&, int width, int height, const string& name, bool covered = true,
      optional<double> defaultLight = none);
  LevelBuilder(RandomGen&, int width, int height, const string& name, bool covered = true);
  
  LevelBuilder(LevelBuilder&&);
  ~LevelBuilder();

  /** Returns a given square.*/
  WSquare modSquare(Vec2);

  /** Checks if it's possible to put a creature on given square.*/
  bool canPutCreature(Vec2, WCreature);

  /** Puts a creatue on a given square. If the square is later changed to something else, the creature remains.*/
  void putCreature(Vec2, PCreature);

  /** Puts items on a given square. If the square is later changed to something else, the items remain.*/
  void putItems(Vec2, vector<PItem> items);
  bool canPutItems(Vec2);

  /** Sets the message displayed when the player first enters the level.*/
  void setMessage(const string& message);

  /** Builds the level. The level will keep reference to the model.
      \paramname{surface} tells if this level is on the Earth surface.*/
  PLevel build(WModel, LevelMaker*, LevelId);

  /** Checks if the given square has an attribute.*/
  bool hasAttrib(Vec2 pos, SquareAttrib attr);

  /** Adds attribute to given square. The attribute will remain if the square is changed.*/
  void addAttrib(Vec2 pos, SquareAttrib attr);

  void putFurniture(Vec2 pos, FurnitureFactory&, optional<SquareAttrib> = none);
  void putFurniture(Vec2 pos, FurnitureParams, optional<SquareAttrib> = none);
  void putFurniture(Vec2 pos, FurnitureType, optional<SquareAttrib> = none);
  void resetFurniture(Vec2 pos, FurnitureType, optional<SquareAttrib> = none);
  void resetFurniture(Vec2 pos, FurnitureParams, optional<SquareAttrib> = none);
  bool canPutFurniture(Vec2 pos, FurnitureLayer);
  void removeFurniture(Vec2 pos, FurnitureLayer);
  void removeAllFurniture(Vec2 pos);
  optional<FurnitureType> getFurnitureType(Vec2 pos, FurnitureLayer);
  bool isFurnitureType(Vec2 pos, FurnitureType);
  WConstFurniture getFurniture(Vec2 pos, FurnitureLayer);

  void setLandingLink(Vec2 pos, StairKey);

  /** Removes attribute from given square.*/
  void removeAttrib(Vec2 pos, SquareAttrib attr);

  bool canNavigate(Vec2 pos, const MovementType&);

  /** Sets the height of the given square.*/
  void setHeightMap(Vec2 pos, double h);

  /** Returns the height of the given square.*/
  double getHeightMap(Vec2 pos);

  Rectangle toGlobalCoordinates(Rectangle);
  vector<Vec2> toGlobalCoordinates(vector<Vec2>);

  /** Adds a collective to the level and initializes it.*/
  void addCollective(CollectiveBuilder*);

  /** Sets the cover of the square. The value will remain if square is changed.*/
  void setCovered(Vec2, bool covered);
  void setSunlight(Vec2, double);

  void setNoDiagonalPassing();

  void setUnavailable(Vec2);
 
  enum Rot { CW0, CW1, CW2, CW3};

  void pushMap(Rectangle bounds, Rot);
  void popMap();

  RandomGen& getRandom();
  
  private:
  Vec2 transform(Vec2);
  SquareArray squares;
  Table<bool> unavailable;
  Table<double> heightMap;
  Table<double> dark;
  vector<CollectiveBuilder*> collectives;
  Table<bool> covered;
  Table<double> sunlight;
  Table<EnumSet<SquareAttrib>> attrib;
  vector<pair<PCreature, Vec2>> creatures;
  Table<vector<PItem>> items;
  FurnitureArray furniture;
  string name;
  vector<Vec2::LinearMap> mapStack;
  ProgressMeter* progressMeter = nullptr;
  RandomGen& random;
  bool noDiagonalPassing = false;
};
