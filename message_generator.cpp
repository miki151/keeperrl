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

static string addName(const string& s, const string& n) {
  if (n.size() > 0)
    return s + " " + n;
  else
    return s;
}

static void addThird(const Creature* c, MsgType type, const string& param) {
  string msg, unseenMsg;
  auto game = c->getGame();
  if (!game)
    return;
  auto factory = game->getContentFactory();
  switch (type) {
    case MsgType::ARE: msg = c->getName().the() + " is " + param; break;
    case MsgType::YOUR: msg = c->getName().the() + "'s " + param; break;
    case MsgType::FEEL: msg = c->getName().the() + " looks " + param; break;
    case MsgType::FALL_ASLEEP: msg = (c->getName().the() + " falls asleep") +
                               (param.size() > 0 ? " on the " + param : ".");
                               unseenMsg = "You hear snoring."; break;
    case MsgType::WAKE_UP: msg = c->getName().the() + " wakes up."; break;
    case MsgType::DIE:
      if (c->isAffected(LastingEffect::FROZEN))
        msg = c->getName().the() + " shatters into a thousand pieces!";
      else
        msg = c->getName().the() + " is " +
          c->getAttributes().getDeathDescription(factory) + "!";
      break;
    case MsgType::TELE_APPEAR: msg = c->getName().the() + " appears out of nowhere!"; break;
    case MsgType::TELE_DISAPPEAR: msg = c->getName().the() + " suddenly disappears!"; break;
    case MsgType::BLEEDING_STOPS: msg = c->getName().the() + "'s bleeding stops."; break;
    case MsgType::DIE_OF: msg = c->getName().the() +
                          " dies" + (param.empty() ? string(".") : " of " + param); break;
    case MsgType::MISS_ATTACK: msg = c->getName().the() + addName(" misses", param); break;
    case MsgType::MISS_THROWN_ITEM: msg = param + " misses " + c->getName().the(); break;
    case MsgType::MISS_THROWN_ITEM_PLURAL: msg = param + " miss " + c->getName().the(); break;
    case MsgType::HIT_THROWN_ITEM: msg = param + " hits " + c->getName().the(); break;
    case MsgType::HIT_THROWN_ITEM_PLURAL: msg = param + " hit " + c->getName().the(); break;
    case MsgType::ITEM_CRASHES: msg = param + " crashes on " + c->getName().the(); break;
    case MsgType::ITEM_CRASHES_PLURAL: msg = param + " crash on " + c->getName().the(); break;
    case MsgType::GET_HIT_NODAMAGE: msg = "The attack is harmless."; break;
    case MsgType::COLLAPSE: msg = c->getName().the() + " collapses."; break;
    case MsgType::FALL: msg = c->getName().the() + " falls " + param; break;
    case MsgType::TRIGGER_TRAP: msg = c->getName().the() + " triggers something."; break;
    case MsgType::DISARM_TRAP: msg = c->getName().the() + " disarms a " + param; break;
    case MsgType::PANIC: msg = c->getName().the() + " panics."; break;
    case MsgType::RAGE: msg = c->getName().the() + " is enraged."; break;
    case MsgType::CRAWL: msg = c->getName().the() + " is crawling"; break;
    case MsgType::STAND_UP: msg = c->getName().the() + " is back on " +
                            his(c->getAttributes().getGender()) + " feet";
                            break;
    case MsgType::TURN_INVISIBLE: msg = c->getName().the() + " disappears!"; break;
    case MsgType::TURN_VISIBLE: msg = c->getName().the() + " appears out of nowhere!"; break;
    case MsgType::ENTER_PORTAL: msg = c->getName().the() + " disappears in the portal."; break;
    case MsgType::HAPPENS_TO: msg = param + " " + c->getName().the(); break;
    case MsgType::BURN: msg = c->getName().the() + " burns in the " + param; unseenMsg = "You hear a horrible scream"; break;
    case MsgType::DROWN: msg = c->getName().the() + " drowns in the " + param; unseenMsg = "You hear a loud splash" ;break;
    case MsgType::SET_UP_TRAP: msg = c->getName().the() + " sets up the trap"; break;
    case MsgType::KILLED_BY: msg = c->getName().the() + " is "+
        c->getAttributes().getDeathDescription(factory) + " by " + param; break;
    case MsgType::TURN: msg = c->getName().the() + " turns " + param; break;
    case MsgType::BECOME: msg = c->getName().the() + " becomes " + param; break;
    case MsgType::COPULATE: msg = c->getName().the() + " copulates " + param; break;
    case MsgType::CONSUME: msg = c->getName().the() + " absorbs " + param; break;
    case MsgType::BREAK_FREE:
        if (param.empty())
          msg = c->getName().the() + " breaks free";
        else
          msg = c->getName().the() + " breaks free from " + param;
        break;
    case MsgType::PRAY: msg = c->getName().the() + " prays to " + param; break;
    case MsgType::SACRIFICE: msg = c->getName().the() + " makes a sacrifice to " + param; break;
    default: break;
  }
  if (!msg.empty()) {
    c->message(msg);
    c->getPosition().unseenMessage(unseenMsg);
  }
}

static void addThird(const Creature* c, const string& param) {
  c->message(c->getName().the() + " " + param);
}

static void addSecond(const Creature* c, const string& param) {
  c->secondPerson("You " + param);
}

static void addSecond(const Creature* c, MsgType type, const string& param) {
  PROFILE;
  string msg;
  switch (type) {
    case MsgType::ARE: msg = "You are " + param; break;
    case MsgType::YOUR: msg = "Your " + param; break;
    case MsgType::FEEL: msg = "You feel " + param; break;
    case MsgType::WAKE_UP: msg = "You wake up."; break;
    case MsgType::FALL: msg = "You fall " + param; break;
    case MsgType::FALL_ASLEEP: msg = "You fall asleep" + (param.size() > 0 ? " on the " + param : "."); break;
    case MsgType::DIE:
      if (c->isAffected(LastingEffect::FROZEN))
        c->secondPerson("You shatter into a thousand pieces!!");
      else
        c->secondPerson("You die!!");
      break;
    case MsgType::TELE_DISAPPEAR: msg = "You are standing somewhere else!"; break;
    case MsgType::BLEEDING_STOPS: msg = "Your bleeding stops."; break;
    case MsgType::DIE_OF: msg = "You die" + (param.empty() ? string(".") : " of " + param); break;
    case MsgType::MISS_ATTACK: msg = "You miss " + param; break;
    case MsgType::MISS_THROWN_ITEM: msg = param + " misses you"; break;
    case MsgType::MISS_THROWN_ITEM_PLURAL: msg = param + " miss you"; break;
    case MsgType::HIT_THROWN_ITEM: msg = param + " hits you"; break;
    case MsgType::HIT_THROWN_ITEM_PLURAL: msg = param + " hit you"; break;
    case MsgType::ITEM_CRASHES: msg = param + " crashes on you."; break;
    case MsgType::ITEM_CRASHES_PLURAL: msg = param + " crash on you."; break;
    case MsgType::GET_HIT_NODAMAGE: msg = "The attack is harmless."; break;
    case MsgType::COLLAPSE: msg = "You collapse."; break;
    case MsgType::TRIGGER_TRAP: msg = "You trigger something."; break;
    case MsgType::DISARM_TRAP: msg = "You disarm the " + param; break;
    case MsgType::ATTACK_SURPRISE: msg = "You sneak attack " + param; break;
    case MsgType::PANIC:
          msg = !c->isAffected(LastingEffect::HALLU) ? "You are suddenly very afraid" : "You freak out completely"; break;
    case MsgType::RAGE:
          msg = !c->isAffected(LastingEffect::HALLU) ?"You are suddenly very angry" : "This will be a very long trip."; break;
    case MsgType::CRAWL: msg = "You are crawling"; break;
    case MsgType::STAND_UP: msg = "You are back on your feet"; break;
    case MsgType::CAN_SEE_HIDING: msg = param + " can see you hiding"; break;
    case MsgType::TURN_INVISIBLE: msg = "You can see through yourself!"; break;
    case MsgType::TURN_VISIBLE: msg = "You are no longer invisible"; break;
    case MsgType::ENTER_PORTAL: msg = "You enter the portal"; break;
    case MsgType::HAPPENS_TO: msg = param + " you."; break;
    case MsgType::BURN: msg = "You burn in the " + param; break;
    case MsgType::DROWN: msg = "You drown in the " + param; break;
    case MsgType::SET_UP_TRAP: msg = "You set up the trap"; break;
    case MsgType::KILLED_BY: msg = "You are killed by " + param; break;
    case MsgType::TURN: msg = "You turn " + param; break;
    case MsgType::BECOME: msg = "You become " + param; break;
    case MsgType::BREAK_FREE:
        if (param.empty())
          msg = "You break free";
        else
          msg = "You break free from " + param;
        break;
    case MsgType::PRAY: msg = "You pray to " + param; break;
    case MsgType::COPULATE: msg = "You copulate " + param; break;
    case MsgType::CONSUME: msg = "You absorb " + param; break;
    case MsgType::SACRIFICE: msg = "You make a sacrifice to " + param; break;
    default: break;
  }
  c->message(PlayerMessage(msg, MessagePriority::HIGH));
}

static void addBoulder(const Creature* c, MsgType type, const string& param) {
  string msg, unseenMsg;
  switch (type) {
    case MsgType::BURN: msg = c->getName().the() + " burns in the " + param; break;
    case MsgType::DROWN: msg = c->getName().the() + " falls into the " + param;
                         unseenMsg = "You hear a loud splash"; break;
    case MsgType::KILLED_BY: msg = c->getName().the() + " is destroyed by " + param; break;
    case MsgType::ENTER_PORTAL: msg = c->getName().the() + " disappears in the portal."; break;
    default: break;
  }
  if (!msg.empty()) {
    c->message(msg);
    c->getPosition().unseenMessage(unseenMsg);
  }
}

static void addKraken(const Creature* c, MsgType type, const string& param) {
  string msg;
  switch (type) {
    case MsgType::KILLED_BY:
      msg = param + "cuts the kraken's tentacle";
      break;
    case MsgType::DIE:
    case MsgType::DIE_OF:
      break;;
    default:
      addThird(c, type, param);
      break;
  }
  if (!msg.empty())
    c->message(msg);
}

void MessageGenerator::add(const Creature* c, MsgType msg, const string& param) {
  switch (type) {
    case SECOND_PERSON:
      addSecond(c, msg, param);
      break;
    case THIRD_PERSON:
      addThird(c, msg, param);
      break;
    case BOULDER:
      addBoulder(c, msg, param);
      break;
    case KRAKEN:
      addKraken(c, msg, param);
      break;
    default:
      break;
  }
}

void MessageGenerator::add(const Creature* c, const string& param) {
  switch (type) {
    case SECOND_PERSON:
      addSecond(c, param);
      break;
    case THIRD_PERSON:
      addThird(c, param);
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

string MessageGenerator::getEnemyName(const Creature* c) {
  if (type == SECOND_PERSON)
    return "you";
  else
    return c->getName().the();
}
