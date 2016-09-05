#include "stdafx.h"
#include "destroy_action.h"
#include "sound.h"

const char* DestroyAction::getVerbSecondPerson(Value value) {
  switch (value) {
    case BASH: return "bash";
    case EAT: return "eat";
    case DESTROY: return "destroy";
    case CUT: return "cut";
  }
}

const char* DestroyAction::getVerbThirdPerson(Value value) {
  switch (value) {
    case BASH: return "bashes";
    case EAT: return "eats";
    case DESTROY: return "destroys";
    case CUT: return "cut";
  }
}

const char*DestroyAction::getIsDestroyed(DestroyAction::Value value) {
   switch (value) {
    case BASH: return "is destroyed";
    case EAT: return "is completely devoured";
    case DESTROY: return "is destroyed";
    case CUT: return "falls";
  }
}

const char* DestroyAction::getSoundText(Value value) {
  switch (value) {
    case BASH: return "BANG!";
    case EAT: return "You hear chewing.";
    case DESTROY:
    case CUT:  return "CRASH";
  }
}

Sound DestroyAction::getSound(DestroyAction::Value value) {
   switch (value) {
    case BASH:
    case EAT:
    case DESTROY: return SoundId::BANG_DOOR;
    case CUT: return SoundId::TREE_CUTTING;
  }
}

