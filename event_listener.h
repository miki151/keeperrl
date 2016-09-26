#pragma once

#include "util.h"
#include "event_generator.h"
#include "position.h"
#include "enum_variant.h"

class Model;
class Technology;
class Collective;

enum class EventId {
  MOVED,
  KILLED,
  PICKED_UP,
  DROPPED,
  ITEMS_APPEARED,
  ITEMS_THROWN,
  EXPLOSION,
  CONQUERED_ENEMY,
  WON_GAME,
  TECHBOOK_READ,
  ALARM,
  TORTURED,
  SURRENDERED,
  TRAP_TRIGGERED,
  TRAP_DISARMED,
  FURNITURE_DESTROYED,
  EQUIPED,
  CREATURE_EVENT,
  POSITION_DISCOVERED
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

struct CreatureEvent {
  Creature* creature;
  string message;
};

}

class GameEvent : public EnumVariant<EventId, TYPES(Creature*, Position, Technology*, Collective*,
    EventInfo::CreatureEvent, EventInfo::Attacked, EventInfo::ItemsHandled, EventInfo::ItemsAppeared,
    EventInfo::ItemsThrown, EventInfo::TrapDisarmed),
    ASSIGN(Creature*, EventId::MOVED),
    ASSIGN(Position, EventId::EXPLOSION, EventId::ALARM, EventId::TRAP_TRIGGERED, EventId::FURNITURE_DESTROYED,
        EventId::POSITION_DISCOVERED),
    ASSIGN(Technology*, EventId::TECHBOOK_READ),
    ASSIGN(Collective*, EventId::CONQUERED_ENEMY),
    ASSIGN(EventInfo::CreatureEvent, EventId::CREATURE_EVENT),
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
  virtual ~EventListener();

  void subscribeTo(Model*);
  void unsubscribe();

  virtual void onEvent(const GameEvent&) = 0;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  Generator* SERIAL(generator) = nullptr;
};

