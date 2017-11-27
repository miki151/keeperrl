#pragma once

#include "stdafx.h"
#include "msg_type.h"
#include "my_containers.h"
#include "owner_pointer.h"

class PlayerMessage;

class MessageGenerator {
  public:
  enum Type { SECOND_PERSON, THIRD_PERSON, BOULDER, KRAKEN, NONE };
  MessageGenerator(Type);
  void add(WConstCreature, MsgType, const string&);
  void add(WConstCreature, const string&);
  void addThirdPerson(WConstCreature, const PlayerMessage&);
  void addSecondPerson(WConstCreature, const PlayerMessage&);
  string getEnemyName(WConstCreature);

  SERIALIZATION_DECL(MessageGenerator)

  private:
  Type SERIAL(type);
};
