#ifndef _MODEL_BUILDER_H

#include "model.h"
#include "village_control.h"
#include "collective_config.h"

class Level;

class ModelBuilder {
  public:

  /** Generates levels and all game entities for a collective game. */
  static Model* collectiveModel(ProgressMeter&, Options*, View*, const string& worldName);

  static Model* splashModel(ProgressMeter&, View*, const string& splashPath);
};


#endif
