#include "stdafx.h"
#include "body_part.h"
#include "t_string.h"

TString getName(BodyPart part) {
  switch (part) {
    case BodyPart::LEG: return TStringId("LEG_BODY_PART");
    case BodyPart::ARM: return TStringId("ARM_BODY_PART");
    case BodyPart::WING: return TStringId("WING_BODY_PART");
    case BodyPart::HEAD: return TStringId("HEAD_BODY_PART");
    case BodyPart::TORSO: return TStringId("TORSO_BODY_PART");
    case BodyPart::BACK: return TStringId("BACK_BODY_PART");
  }
}

TString getPluralText(BodyPart part, int num) {
  if (num == 1)
    return TSentence("A_ARTICLE", getName(part));
  else
    return TSentence("BODY_PART_NUMBER", toText(num), makePlural(getName(part)));
}
