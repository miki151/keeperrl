#pragma once

#include "util.h"

RICH_ENUM2(std::uint8_t,
    ViewObjectAction,
    LOCK_DOOR,
    UNLOCK_DOOR,
    WRITE_ON_BOARD,
    SWAP_POSITION,
    PUSH,
    ATTACK,
    ATTACK_USING_WEAPON,
    ATTACK_USING_HEAD,
    ATTACK_USING_TORSO,
    ATTACK_USING_ARM,
    ATTACK_USING_WING,
    ATTACK_USING_LEG,
    ATTACK_USING_BACK,
    PET,
    SKIP_TURN,
    CHAT,
    ORDER_CAPTURE,
    CANCEL_CAPTURE_ORDER,
    ADD_TO_TEAM,
    SWITCH_LEADER,
    REMOVE_FROM_TEAM,
    MOVE_NOW
);

const char* getText(ViewObjectAction);
