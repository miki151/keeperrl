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
  WCreature victim;
  WCreature attacker;
};

struct ItemsHandled {
  WCreature creature;
  vector<WItem> items;
};

struct ItemsAppeared {
  Position position;
  vector<WItem> items;
};

struct ItemsThrown {
  Level* level;
  vector<WItem> items;
  vector<Vec2> trajectory;
};

struct TrapDisarmed {
  Position position;
  WCreature creature;
};

struct CreatureEvent {
  WCreature creature;
  string message;
};

struct FurnitureEvent {
  Position position;
  FurnitureLayer layer;
};

}

class GameEvent : public EnumVariant<EventId, TYPES(WCreature, Position, Technology*, WCollective,
    EventInfo::CreatureEvent, EventInfo::Attacked, EventInfo::ItemsHandled, EventInfo::ItemsAppeared,
    EventInfo::ItemsThrown, EventInfo::TrapDisarmed, EventInfo::FurnitureEvent),
    ASSIGN(WCreature, EventId::MOVED),
    ASSIGN(Position, EventId::EXPLOSION, EventId::ALARM, EventId::TRAP_TRIGGERED,
        EventId::POSITION_DISCOVERED),
    ASSIGN(Technology*, EventId::TECHBOOK_READ),
    ASSIGN(WCollective, EventId::CONQUERED_ENEMY),
    ASSIGN(EventInfo::CreatureEvent, EventId::CREATURE_EVENT),
    ASSIGN(EventInfo::Attacked, EventId::KILLED, EventId::TORTURED, EventId::SURRENDERED),
    ASSIGN(EventInfo::ItemsHandled, EventId::PICKED_UP, EventId::DROPPED, EventId::EQUIPED),
    ASSIGN(EventInfo::ItemsAppeared, EventId::ITEMS_APPEARED),
    ASSIGN(EventInfo::ItemsThrown, EventId::ITEMS_THROWN),
    ASSIGN(EventInfo::TrapDisarmed, EventId::TRAP_DISARMED),
    ASSIGN(EventInfo::FurnitureEvent, EventId::FURNITURE_DESTROYED) > {
  using EnumVariant::EnumVariant;
};

class EventListener {
  public:
  typedef EventGenerator<EventListener> Generator;

  EventListener();
  EventListener(Model*);
  EventListener(const EventListener&) = delete;
  EventListener(EventListener&&) = delete;
  virtual ~EventListener();

  void subscribeTo(Model*);
  void unsubscribe();
  bool isSubscribed() const;

  virtual void onEvent(const GameEvent&) = 0;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  Generator* SERIAL(generator) = nullptr;
};

