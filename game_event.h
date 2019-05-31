#pragma once

#include "util.h"
#include "position.h"
#include "fx_info.h"
#include "view_id.h"
#include "furniture_type.h"
#include "tech_id.h"

class Model;
class Technology;
class Collective;

namespace EventInfo {

  struct CreatureMoved {
    Creature* creature = nullptr;
  };

  struct CreatureKilled {
    Creature* victim = nullptr;
    Creature* attacker = nullptr;
  };

  struct ItemsPickedUp {
    Creature* creature = nullptr;
    vector<Item*> items;
  };

  struct ItemsDropped {
    Creature* creature = nullptr;
    vector<Item*> items;
  };

  struct ItemsAppeared {
    Position position;
    vector<Item*> items;
  };

  struct Projectile {
    optional<FXInfo> fx;
    optional<ViewId> viewId;
    Position begin;
    Position end;
    optional<SoundId> sound;
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
    Creature* victim = nullptr;
    Creature* torturer = nullptr;
  };

  struct CreatureStunned {
    Creature* victim = nullptr;
    Creature* attacker = nullptr;
  };

  struct CreatureAttacked {
    Creature* victim = nullptr;
    Creature* attacker = nullptr;
    AttrType damageAttr;
  };

  struct TrapTriggered {
    Position pos;
  };

  struct TrapDisarmed {
    Position pos;
    Creature* creature = nullptr;
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
    Creature* creature = nullptr;
    vector<Item*> items;
  };

  struct CreatureEvent {
    Creature* creature = nullptr;
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
