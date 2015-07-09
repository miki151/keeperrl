#ifndef _MODEL_BUILDER_H

#include "village_control.h"
#include "stair_key.h"

class Level;
class Model;
class ProgressMeter;
class Options;
class View;

enum class ExtraLevelId;

class ModelBuilder {
  public:

  /** Generates levels and all game entities for a collective game. */
  static PModel collectiveModel(ProgressMeter&, Options*, View*, const string& worldName);

  static PModel splashModel(ProgressMeter&, View*, const string& splashPath);

  static int getPigstyPopulationIncrease();
  static int getStatuePopulationIncrease();
  static int getThronePopulationIncrease();

  private:
  static PModel tryCollectiveModel(ProgressMeter&, Options*, View*, const string& worldName);
  static Level* makeExtraLevel(ProgressMeter&, Model*, ExtraLevelId, StairKey);

};


#endif
