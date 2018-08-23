#include "animation_id.h"


const char* getFileName(AnimationId id) {
  switch (id) {
    case AnimationId::ATTACK:
      return "attack.png";
    case AnimationId::DEATH:
      return "death.png";
  }
}

milliseconds getDuration(AnimationId id) {
  switch (id) {
    case AnimationId::ATTACK:
      return milliseconds{300};
    case AnimationId::DEATH:
      return milliseconds{800};
  }
}

int getNumFrames(AnimationId id) {
  switch (id) {
    case AnimationId::ATTACK:
      return 3;
    case AnimationId::DEATH:
      return 8;
  }
}
