#pragma once

#include "stdafx.h"
#include "util.h"
#include "buff_id.h"
#include "game_time.h"

class TimeInterval;
class ContentFactory;
class Creature;
struct Color;


using LastingOrBuff = variant<LastingEffect, BuffId>;

string getName(const LastingOrBuff&, const ContentFactory*);
string getDescription(const LastingOrBuff&, const ContentFactory*);
string getAdjective(const LastingOrBuff&, const ContentFactory*);
int getPrice(const LastingOrBuff&, const ContentFactory*);
bool addEffect(const LastingOrBuff&, Creature*, optional<TimeInterval> duration, bool msg = true);
bool addEffect(const LastingOrBuff&, Creature*, optional<TimeInterval> duration, GlobalTime, const ContentFactory*);
bool removeEffect(const LastingOrBuff&, Creature*, bool msg = true);
bool addPermanentEffect(const LastingOrBuff&, Creature*, bool msg = true);
bool removePermanentEffect(const LastingOrBuff&, Creature*, bool msg = true);
bool isConsideredBad(const LastingOrBuff&, const ContentFactory*);
bool isAffected(const Creature*, const LastingOrBuff&);
bool isAffected(const Creature*, const LastingOrBuff&, GlobalTime);
bool isAffectedPermanently(const Creature*, const LastingOrBuff&);
bool shouldAIApply(const LastingOrBuff&, bool enemy, const Creature*);
Color getColor(const LastingOrBuff&, const ContentFactory*);
optional<string> getHatedGroupName(const LastingOrBuff&, const ContentFactory*);
void serialize(PrettyInputArchive& ar1, LastingOrBuff&, const unsigned int);
