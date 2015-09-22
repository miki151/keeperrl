#ifndef _PLAYER_MESSAGE_H
#define _PLAYER_MESSAGE_H

#include "util.h"
#include "unique_entity.h"
#include "position.h"

class Location;

class PlayerMessage : public UniqueEntity<PlayerMessage> {
  public:
  enum Priority { NORMAL, HIGH, CRITICAL };

  PlayerMessage(const string&, Priority = NORMAL);
  PlayerMessage(const char*, Priority = NORMAL);

  static PlayerMessage announcement(const string& title, const string& text);

  PlayerMessage& setPosition(Position);
  optional<Position> getPosition() const;
  optional<string> getAnnouncementTitle() const;

  PlayerMessage& setCreature(UniqueEntity<Creature>::Id);
  optional<UniqueEntity<Creature>::Id> getCreature() const;

  PlayerMessage& setLocation(const Location*);
  const Location* getLocation() const;

  bool isClickable() const;

  string getText() const;
  Priority getPriority() const;
  double getFreshness() const;
  void setFreshness(double);
  
  SERIALIZATION_DECL(PlayerMessage);

  private:
  PlayerMessage(const string& title, const string& text);
  string SERIAL(text);
  Priority SERIAL(priority);
  double SERIAL(freshness);
  optional<string> SERIAL(announcementTitle);
  optional<Position> SERIAL(position);
  optional<UniqueEntity<Creature>::Id> SERIAL(creature);
  const Location* SERIAL(location) = nullptr;
};

#endif
