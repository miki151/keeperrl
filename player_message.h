#pragma once

#include "util.h"
#include "unique_entity.h"
#include "position.h"
#include "t_string.h"

class GuiFactory;
class View;

RICH_ENUM(MessagePriority, NORMAL, HIGH, CRITICAL);

class PlayerMessage : public UniqueEntity<PlayerMessage> {
  public:
  PlayerMessage(const TString&, MessagePriority = MessagePriority::NORMAL);
  PlayerMessage(const TSentence&, MessagePriority = MessagePriority::NORMAL);
  PlayerMessage(const TStringId&, MessagePriority = MessagePriority::NORMAL);
  PlayerMessage(const char*, MessagePriority = MessagePriority::NORMAL) = delete;

  static void presentMessages(View*, const vector<PlayerMessage>&);

  PlayerMessage& setPosition(Position);
  optional<Position> getPosition() const;

  PlayerMessage& setCreature(UniqueEntity<Creature>::Id);
  optional<UniqueEntity<Creature>::Id> getCreature() const;

  bool isClickable() const;

  const TString& getText() const;
  string getTranslatedText(View*) const;
  string getTranslatedText(GuiFactory*) const;
  MessagePriority getPriority() const;
  double getFreshness() const;
  void setFreshness(double);

  int getHash() const;

  SERIALIZATION_DECL(PlayerMessage);

  TString SERIAL(text);
  
  private:
  MessagePriority SERIAL(priority);
  double SERIAL(freshness);
  optional<string> SERIAL(announcementTitle);
  optional<Position> SERIAL(position);
  optional<UniqueEntity<Creature>::Id> SERIAL(creature);
};

CEREAL_CLASS_VERSION(PlayerMessage, 1);