#pragma once

#include "player_message.h"

class MessageBuffer {
  public:
  vector<PlayerMessage> SERIAL(current);
  vector<PlayerMessage> SERIAL(history);
  SERIALIZE_ALL(current, history)
};
