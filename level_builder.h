#pragma once

#include "util.h"
#include "level.h"
#include "square_array.h"
#include "furniture_array.h"
#include "view_object.h"
#include "square_attrib.h"
#include "tile_gas_type.h"
#include "creature_list.h"

class ProgressMeter;
class Model;
class LevelMaker;
class Square;
class FurnitureFactory;
class CollectiveBuilder;
class ContentFactory;

class LevelBuilder {
  public:
  /** Constructs a builder with given size and name. */
  LevelBuilder(ProgressMeter*, RandomGen&, ContentFactory*, int width, int height,
      bool covered = true, optional<double> defaultLight = none);
  LevelBuilder(RandomGen&, ContentFactory*, int width, int height, bool covered = true);
  
  LevelBuilder(LevelBuilder&&);
  ~LevelBuilder();

  /** Returns a given square.*/
  Square* modSquare(Vec2);

  /** Checks if it's possible to put a creature on given square.*/
  bool canPutCreature(Vec2, Creature*);

  /** Puts a creatue on a given square. If the square is later changed to something else, the creature remains.*/
  void putCreature(Vec2, PCreature);

  /** Puts items on a given square. If the square is later changed to something else, the items remain.*/
  void putItems(Vec2, vector<PItem> items);
  bool canPutItems(Vec2);

  /** Sets the message displayed when the player first enters the level.*/
  void setMessage(const string& message);

  /** Builds the level. The level will keep reference to the model.
      \paramname{surface} tells if this level is on the Earth surface.*/
  PLevel build(const ContentFactory*, WModel, LevelMaker*, LevelId);

  /** Checks if the given square has an attribute.*/
  bool hasAttrib(Vec2 pos, SquareAttrib attr);

  /** Adds attribute to given square. The attribute will remain if the square is changed.*/
  void addAttrib(Vec2 pos, SquareAttrib attr);

  void putFurniture(Vec2 pos, FurnitureList&, TribeId, optional<SquareAttrib> = none);
  void putFurniture(Vec2 pos, FurnitureParams, optional<SquareAttrib> = none);
  void putFurniture(Vec2 pos, FurnitureType, TribeId tribe, optional<SquareAttrib> attrib = none);
  void putFurniture(Vec2 pos, FurnitureType, optional<SquareAttrib> = none);
  void resetFurniture(Vec2 pos, FurnitureType, optional<SquareAttrib> = none);
  void resetFurniture(Vec2 pos, FurnitureParams, optional<SquareAttrib> = none);
  bool canPutFurniture(Vec2 pos, FurnitureLayer);
  void removeFurniture(Vec2 pos, FurnitureLayer);
  void removeAllFurniture(Vec2 pos);
  optional<FurnitureType> getFurnitureType(Vec2 pos, FurnitureLayer);
  bool isFurnitureType(Vec2 pos, FurnitureType);
  const Furniture* getFurniture(Vec2 pos, FurnitureLayer);

  void setLandingLink(Vec2 pos, StairKey);

  /** Removes attribute from given square.*/
  void removeAttrib(Vec2 pos, SquareAttrib attr);

  bool canNavigate(Vec2 pos, const MovementType&);

  /** Sets the height of the given square.*/
  void setHeightMap(Vec2 pos, double h);

  /** Returns the height of the given square.*/
  double getHeightMap(Vec2 pos);

  Rectangle toGlobalCoordinates(Rectangle);
  vector<Vec2> toGlobalCoordinates(const vector<Vec2>&);

  /** Adds a collective to the level and initializes it.*/
  void addCollective(CollectiveBuilder*);

  /** Sets the cover of the square. The value will remain if square is changed.*/
  void setCovered(Vec2, bool state);

  /** Sets building flag for the purpose of building level. Buildings are recomputed after world generation
   * using the roof support algorithm for the sake of game mechanics */
  void setBuilding(Vec2, bool state);

  void setSunlight(Vec2, double);

  void setNoDiagonalPassing();

  void setUnavailable(Vec2);

  void addPermanentGas(TileGasType, Vec2);

  void setMountainLevel(Vec2, int);
 
  enum Rot { CW0, CW1, CW2, CW3};

  void pushMap(Rectangle bounds, Rot);
  void popMap();

  RandomGen& getRandom();
  ContentFactory* getContentFactory() const;

  CreatureList wildlife;
  
  private:
  Vec2 transform(Vec2);
  SquareArray squares;
  Table<bool> unavailable;
  Table<double> heightMap;
  Table<double> dark;
  vector<CollectiveBuilder*> collectives;
  Table<bool> covered;
  Table<bool> building;
  Table<double> sunlight;
  Table<EnumSet<SquareAttrib>> attrib;
  vector<pair<PCreature, Vec2>> creatures;
  Table<vector<PItem>> items;
  FurnitureArray furniture;
  vector<Vec2::LinearMap> mapStack;
  ProgressMeter* progressMeter = nullptr;
  RandomGen& random;
  bool noDiagonalPassing = false;
  ContentFactory* contentFactory;
  vector<pair<TileGasType, Vec2>> permanentGas;
  Table<int> mountainLevel;
};
