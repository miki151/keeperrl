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
    PET,
    MOUNT,
    DISMOUNT,
    SKIP_TURN,
    CHAT,
    ORDER_CAPTURE,
    CANCEL_CAPTURE_ORDER,
    ADD_TO_TEAM,
    SWITCH_LEADER,
    REMOVE_FROM_TEAM,
    MOVE_NOW
);

class TStringId;
TStringId getText(ViewObjectAction);
