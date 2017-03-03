#pragma once

#include "util.h"
#include "unique_entity.h"
#include "position.h"

class Location;
class View;

enum class MessagePriority { NORMAL, HIGH, CRITICAL };

class PlayerMessage : public UniqueEntity<PlayerMessage> {
  public:
  PlayerMessage(const string&, MessagePriority = MessagePriority::NORMAL);
  PlayerMessage(const char*, MessagePriority = MessagePriority::NORMAL);

  static void presentMessages(View*, const vector<PlayerMessage>&);
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
  MessagePriority getPriority() const;
  double getFreshness() const;
  void setFreshness(double);

  int getHash() const;

  
  SERIALIZATION_DECL(PlayerMessage);

  private:
  PlayerMessage(const string& title, const string& text);
  string SERIAL(text);
  MessagePriority SERIAL(priority);
  double SERIAL(freshness);
  optional<string> SERIAL(announcementTitle);
  optional<Position> SERIAL(position);
  optional<UniqueEntity<Creature>::Id> SERIAL(creature);
  const Location* SERIAL(location) = nullptr;
};

