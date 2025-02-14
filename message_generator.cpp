#include "message_generator.h"
#include "creature.h"
#include "creature_attributes.h"
#include "creature_name.h"
#include "player_message.h"
#include "game.h"

SERIALIZE_DEF(MessageGenerator, type)
SERIALIZATION_CONSTRUCTOR_IMPL(MessageGenerator)

MessageGenerator::MessageGenerator(MessageGenerator::Type t) : type(t) {
}

static void addThird(const Creature* c, MsgType type, vector<TString> param) {
  optional<TSentence> unseenMsg;
  auto game = c->getGame();
  if (!game)
    return;
  auto factory = game->getContentFactory();
  auto msg = [&] () -> optional<TSentence> {
    param.push_front(c->getName().the());
    switch (type) {
      case MsgType::ARE: return TSentence("IS", param);
      case MsgType::YOUR: return TSentence("HIS", param);
      case MsgType::FALL_ASLEEP:
        unseenMsg = TSentence("YOU_HEAR_SNORING");
        if (!param.empty())
          return TSentence("FALLS_ASLEEP_IN", param);
        else
          return TSentence("FALLS_ASLEEP", param);
      case MsgType::WAKE_UP: return TSentence("WAKES_UP", param);
      case MsgType::DIE:
        if (c->isAffected(LastingEffect::FROZEN))
          return TSentence("SHATTERS", param);
        else
          return TSentence("IS_KILLED", concat(param, {c->getAttributes().getDeathDescription(factory)}));
      case MsgType::TELE_APPEAR: return TSentence("APPEARS", param);
      case MsgType::TELE_DISAPPEAR: return TSentence("DISAPPEARS", param);
      case MsgType::DIE_OF: return TSentence("DIES_OF", param);
      case MsgType::MISS_THROWN_ITEM: return TSentence("ITEM_MISSES", param);
      case MsgType::MISS_THROWN_ITEM_PLURAL: return TSentence("ITEMS_MISS", param);
      case MsgType::HIT_THROWN_ITEM: return TSentence("ITEM_HITS", param);
      case MsgType::HIT_THROWN_ITEM_PLURAL: return TSentence("ITEMS_HIT", param);
      case MsgType::ITEM_CRASHES: return TSentence("ITEM_CRASHES_ON", param);
      case MsgType::ITEM_CRASHES_PLURAL: return TSentence("ITEMS_CRASH_ON", param);
      case MsgType::GET_HIT_NODAMAGE: return TSentence("ATTACK_IS_HARMLESS");
      case MsgType::COLLAPSE: return TSentence("COLLAPSES", param);
      case MsgType::TRIGGER_TRAP: return TSentence("TRIGGERS_SOMETHING", param);
      case MsgType::DISARM_TRAP: return TSentence("DISARMS", param);
      case MsgType::PANIC: return TSentence("PANICS", param);
      case MsgType::RAGE: return TSentence("RAGES", param);
      case MsgType::CRAWL: return TSentence("IS_CRAWLING", param);
      case MsgType::STAND_UP: return TSentence("IS_NO_LONGER_CRAWLING",
          concat(param, {TString(his(c->getAttributes().getGender()))}));
      case MsgType::TURN_INVISIBLE: return TSentence("IS_INVISIBLE", param);
      case MsgType::TURN_VISIBLE: return TSentence("IS_NO_LONGER_INVISIBLE", param);
      case MsgType::ENTER_PORTAL: return TSentence("ENTERS_PORTAL", param);
      case MsgType::BURN:
        unseenMsg = TSentence("YOU_HEAR_HORRIBLE_SCREAM");
        return TSentence("BURNS_IN", param);
      case MsgType::DROWN:
        unseenMsg = TSentence("YOU_HEAR_SOMEONE_DROWNING");
        return TSentence("DROWNS_IN", param);
      case MsgType::KILLED_BY: return TSentence("IS_KILLED_BY",
          concat(param, {c->getAttributes().getDeathDescription(factory)}));
      case MsgType::BECOME: return TSentence("BECOMES", param);
      case MsgType::COPULATE: return TSentence("COPULATES", param);
      case MsgType::CONSUME: return TSentence("ABSORBS", param);
      case MsgType::BREAK_FREE:
          if (param.empty())
            return TSentence("BREAKS_FREE", param);
          else
            return TSentence("BREAKS_FREE_FROM", param);
      default: return none;
    }
  }();
  if (msg)
    c->message(*msg);
  if (unseenMsg)
    c->getPosition().unseenMessage(*unseenMsg);
}

static void addSecond(const Creature* c, MsgType type, vector<TString> param) {
  PROFILE;
  auto msg = [&] () -> optional<TSentence> {
    auto factory = c->getGame()->getContentFactory();
    switch (type) {
      case MsgType::ARE: return TSentence("YOU_ARE", param);
      case MsgType::YOUR: return TSentence("YOUR", param);
      case MsgType::WAKE_UP: return TSentence("YOU_WAKE_UP");
      case MsgType::FALL_ASLEEP:
        if (!param.empty())
          return TSentence("YOU_FALL_ASLEEP_IN", param);
        else
          return TSentence("YOU_FALL_ASLEEP");
      case MsgType::DIE:
        if (c->isAffected(LastingEffect::FROZEN))
          return TSentence("YOU_SHATTER");
        else
          return TSentence("YOU_DIE");
      case MsgType::TELE_DISAPPEAR: return TSentence("YOU_ARE_STANDING_SOMEWHERE_ELSE");
      case MsgType::DIE_OF: return TSentence("YOU_DIE_OF", param);
      case MsgType::MISS_THROWN_ITEM: return TSentence("ITEM_MISSES_YOU", param);
      case MsgType::MISS_THROWN_ITEM_PLURAL: return TSentence("ITEMS_MISS_YOU", param);
      case MsgType::HIT_THROWN_ITEM: return TSentence("ITEM_HITS_YOU", param);
      case MsgType::HIT_THROWN_ITEM_PLURAL: return TSentence("ITEMS_HIT_YOU", param);
      case MsgType::ITEM_CRASHES: return TSentence("ITEM_CRASHES_ON_YOU", param);
      case MsgType::ITEM_CRASHES_PLURAL: return TSentence("ITEMS_CRASH_ON_YOU", param);
      case MsgType::GET_HIT_NODAMAGE: return TSentence("ATTACK_ON_YOU_IS_HARMLESS");
      case MsgType::COLLAPSE: return TSentence("YOU_COLLAPSE");
      case MsgType::TRIGGER_TRAP: return TSentence("YOU_TRIGGER_SOMETHING");
      case MsgType::DISARM_TRAP: return TSentence("YOU_DISARM", param);
      case MsgType::PANIC:
        return !c->isAffected(LastingEffect::HALLU) ? TSentence("YOU_AFRAID") : TSentence("YOU_FREAK_OUT");
      case MsgType::RAGE:
        return !c->isAffected(LastingEffect::HALLU) ? TSentence("YOU_ANGRY") : TSentence("YOU_ANGRY_HALLU");
      case MsgType::CRAWL: return TSentence("YOU_CRAWLING");
      case MsgType::STAND_UP: return TSentence("YOU_NO_LONGER_CRAWLING");
      case MsgType::CAN_SEE_HIDING: return TSentence("CAN_SEE_YOU_HIDING", param);
      case MsgType::TURN_INVISIBLE: return TSentence("YOU_INVISIBLE");
      case MsgType::TURN_VISIBLE: return TSentence("YOU_NO_LONGER_INVISIBLE");
      case MsgType::ENTER_PORTAL: return TSentence("YOU_ENTER_PORTAL");
      case MsgType::BURN: return TSentence("YOU_BURN_IN", param);
      case MsgType::DROWN: return TSentence("YOU_DROWN_IN", param);
      case MsgType::KILLED_BY: return TSentence("YOU_KILLED_BY",
          concat(param, {c->getAttributes().getDeathDescription(factory)}));
      case MsgType::BECOME: return TSentence("YOU_BECOME", param);
      case MsgType::BREAK_FREE: return TSentence("YOU_BREAK_FREE");
      case MsgType::COPULATE: return TSentence("YOU_COPULATE", param);
      case MsgType::CONSUME: return TSentence("YOU_ABSORB", param);
      default:
        return none;
    }
  }();
  if (msg)
    c->message(PlayerMessage(*msg, MessagePriority::HIGH));
}

static void addBoulder(const Creature* c, MsgType type, vector<TString> param) {
  optional<TSentence> msg, unseenMsg;
  param.push_front(c->getName().the());
  switch (type) {
    case MsgType::BURN:
      msg = TSentence("BURNS_IN", param);
      break;
    case MsgType::DROWN:
      unseenMsg = TSentence("YOU_HEAR_A_LOUD_SPLASH");
      msg = TSentence("DROWNS_IN", param);
      break;
    case MsgType::KILLED_BY:
      msg = TSentence("IS_DESTROYED_BY", param);
      break;
    default:
      break;
  }
  if (msg)
    c->message(*msg);
  if (unseenMsg)
    c->getPosition().unseenMessage(*unseenMsg);
}

static void addKraken(const Creature* c, MsgType type, vector<TString> param) {
  switch (type) {
    case MsgType::KILLED_BY:
      c->message(TSentence("CUTS_TENTACLE", param));
      break;
    case MsgType::DIE:
    case MsgType::DIE_OF:
      break;
    default:
      addThird(c, type, param);
      break;
  }
}

void MessageGenerator::add(const Creature* c, MsgType msg, TString param) {
  vector<TString> params {param};
  switch (type) {
    case SECOND_PERSON:
      addSecond(c, msg, params);
      break;
    case THIRD_PERSON:
      addThird(c, msg, params);
      break;
    case BOULDER:
      addBoulder(c, msg, params);
      break;
    case KRAKEN:
      addKraken(c, msg, params);
      break;
    default:
      break;
  }
}

void MessageGenerator::add(const Creature* c, MsgType msg) {
  switch (type) {
    case SECOND_PERSON:
      addSecond(c, msg, {});
      break;
    case THIRD_PERSON:
      addThird(c, msg, {});
      break;
    case BOULDER:
      addBoulder(c, msg, {});
      break;
    case KRAKEN:
      addKraken(c, msg, {});
      break;
    default:
      break;
  }
}

void MessageGenerator::addThirdPerson(const Creature* c, const PlayerMessage& msg) {
  if (type == THIRD_PERSON)
    c->message(msg);
}

void MessageGenerator::addSecondPerson(const Creature* c, const PlayerMessage& msg) {
  if (type == SECOND_PERSON)
    c->message(msg);
}

TString MessageGenerator::getEnemyName(const Creature* c) {
  if (type == SECOND_PERSON)
    return TStringId("YOU_ATTACK_TARGET");
  else
    return c->getName().the();
}
