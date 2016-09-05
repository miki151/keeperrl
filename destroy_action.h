#pragma once

class Sound;

class DestroyAction {
  public:
  enum Value { BASH, EAT, DESTROY, CUT };

  static const char* getVerbSecondPerson(Value);
  static const char* getVerbThirdPerson(Value);
  static const char* getIsDestroyed(Value);
  static const char* getSoundText(Value);
  static Sound getSound(Value);
};
