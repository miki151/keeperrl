#include "stdafx.h"
#include "body_part.h"
#include "view_object_action.h"

const char* getName(BodyPart part) {
  switch (part) {
    case BodyPart::LEG: return "leg";
    case BodyPart::ARM: return "arm";
    case BodyPart::WING: return "wing";
    case BodyPart::HEAD: return "head";
    case BodyPart::TORSO: return "torso";
    case BodyPart::BACK: return "back";
  }
}

ViewObjectAction getAttackAction(BodyPart part) {
  switch (part) {
    case BodyPart::LEG: return ViewObjectAction::ATTACK_USING_LEG;
    case BodyPart::ARM: return ViewObjectAction::ATTACK_USING_ARM;
    case BodyPart::WING: return ViewObjectAction::ATTACK_USING_WING;
    case BodyPart::HEAD: return ViewObjectAction::ATTACK_USING_HEAD;
    case BodyPart::TORSO: return ViewObjectAction::ATTACK_USING_TORSO;
    case BodyPart::BACK: return ViewObjectAction::ATTACK_USING_BACK;
  }
}
