#pragma once

#include "util.h"
#include "position.h"
#include "fx_info.h"

class Model;
class Technology;
class Collective;

namespace EventInfo {

  struct CreatureMoved {
    WCreature creature = nullptr;
  };

  struct CreatureKilled {
    WCreature victim = nullptr;
    WCreature attacker = nullptr;
  };

  struct ItemsPickedUp {
    WCreature creature = nullptr;
    vector<WItem> items;
  };

  struct ItemsDropped {
    WCreature creature = nullptr;
    vector<WItem> items;
  };

  struct ItemsAppeared {
    Position position;
    vector<WItem> items;
  };

  struct Projectile {
    optional<FXInfo> fx;
    optional<ViewId> viewId;
    Position begin;
    Position end;
  };

  struct ConqueredEnemy {
    WCollective collective = nullptr;
  };

  struct WonGame {};

  struct RetiredGame {};

  struct TechbookRead {
    TechId technology;
  };

  struct Alarm {
    Position pos;
    bool silent = false;
  };

  struct CreatureTortured {
    WCreature victim = nullptr;
    WCreature torturer = nullptr;
  };

  struct CreatureStunned {
    WCreature victim = nullptr;
    WCreature attacker = nullptr;
  };

  struct CreatureAttacked {
    WCreature victim = nullptr;
    WCreature attacker = nullptr;
  };

  struct TrapTriggered {
    Position pos;
  };

  struct TrapDisarmed {
    Position pos;
    WCreature creature = nullptr;
  };

  struct FurnitureDestroyed {
    Position position;
    FurnitureType type;
    FurnitureLayer layer;
  };

  struct FX {
    Position position;
    FXInfo fx;
    optional<Vec2> direction = none;
  };

  struct ItemsEquipped {
    WCreature creature = nullptr;
    vector<WItem> items;
  };

  struct CreatureEvent {
    WCreature creature = nullptr;
    string message;
  };

  struct VisibilityChanged {
    Position pos;
  };

  struct MovementChanged {
    Position pos;
  };

  class GameEvent : public variant<CreatureMoved, CreatureKilled, ItemsPickedUp, ItemsDropped, ItemsAppeared, Projectile,
      ConqueredEnemy, WonGame, TechbookRead, Alarm, CreatureTortured, CreatureStunned, MovementChanged,
      TrapTriggered, TrapDisarmed, FurnitureDestroyed, ItemsEquipped, CreatureEvent, VisibilityChanged, RetiredGame,
      CreatureAttacked, FX> {
    using variant::variant;
  };

}

class GameEvent : public EventInfo::GameEvent {
  using EventInfo::GameEvent::GameEvent;
};
