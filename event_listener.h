#pragma once

#include "util.h"
#include "event_generator.h"
#include "position.h"
#include "enum_variant.h"

class Model;
class Technology;

enum class EventId {
  MOVED,
  KILLED,
  PICKED_UP,
  DROPPED,
  ITEMS_APPEARED,
  ITEMS_THROWN,
  EXPLOSION,
  WON_GAME,
  TECHBOOK_READ,
  ALARM,
  TORTURED,
  SURRENDERED,
  TRAP_TRIGGERED,
  TRAP_DISARMED,
  SQUARE_DESTROYED,
  EQUIPED
};

namespace EventInfo {

struct Attacked {
  Creature* victim;
  Creature* attacker;
};

struct ItemsHandled {
  Creature* creature;
  vector<Item*> items;
};

struct ItemsAppeared {
  Position position;
  vector<Item*> items;
};

struct ItemsThrown {
  Level* level;
  vector<Item*> items;
  vector<Vec2> trajectory;
};

struct TrapDisarmed {
  Position position;
  Creature* creature;
};

}

class GameEvent : public EnumVariant<EventId, TYPES(Creature*, Position, Technology*, EventInfo::Attacked,
    EventInfo::ItemsHandled, EventInfo::ItemsAppeared, EventInfo::ItemsThrown, EventInfo::TrapDisarmed),
    ASSIGN(Creature*, EventId::MOVED),
    ASSIGN(Position, EventId::EXPLOSION, EventId::ALARM, EventId::TRAP_TRIGGERED, EventId::SQUARE_DESTROYED),
    ASSIGN(Technology*, EventId::TECHBOOK_READ),
    ASSIGN(EventInfo::Attacked, EventId::KILLED, EventId::TORTURED, EventId::SURRENDERED),
    ASSIGN(EventInfo::ItemsHandled, EventId::PICKED_UP, EventId::DROPPED, EventId::EQUIPED),
    ASSIGN(EventInfo::ItemsAppeared, EventId::ITEMS_APPEARED),
    ASSIGN(EventInfo::ItemsThrown, EventId::ITEMS_THROWN),
    ASSIGN(EventInfo::TrapDisarmed, EventId::TRAP_DISARMED)> {
  using EnumVariant::EnumVariant;
};

class EventListener {
  public:
  typedef EventGenerator<EventListener> Generator;

  EventListener();
  EventListener(Model*);
  ~EventListener();

  void subscribeTo(Model*);
  void unsubscribe();

  virtual void onEvent(const GameEvent&) = 0;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  Generator* SERIAL(generator) = nullptr;
};

