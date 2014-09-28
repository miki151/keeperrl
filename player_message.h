#ifndef _PLAYER_MESSAGE_H
#define _PLAYER_MESSAGE_H

#include "util.h"

class PlayerMessage {
  public:
  enum Priority { NORMAL, HIGH, CRITICAL };

  PlayerMessage(const string&, Priority = NORMAL);
  PlayerMessage(const char*, Priority = NORMAL);

  const string& getText() const;
  Priority getPriority() const;
  double getFreshness() const;
  void setFreshness(double);
  
  SERIALIZATION_DECL(PlayerMessage);

  private:
  string SERIAL(text);
  Priority SERIAL(priority);
  double SERIAL(freshness);
};

#endif
