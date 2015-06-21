#ifndef _MODEL_BUILDER_H

#include "village_control.h"
#include "collective_config.h"

class Level;
class Model;
class ProgressMeter;
class Options;
class View;

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

};


#endif
