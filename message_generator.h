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
  void add(const Creature*, MsgType, const string&);
  void add(const Creature*, const string&);
  void addThirdPerson(const Creature*, const PlayerMessage&);
  void addSecondPerson(const Creature*, const PlayerMessage&);
  string getEnemyName(const Creature*);

  SERIALIZATION_DECL(MessageGenerator)

  private:
  Type SERIAL(type);
};
