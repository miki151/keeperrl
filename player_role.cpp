#include "stdafx.h"
#include "player_role.h"

const char* getName(PlayerRole role) {
  switch (role) {
    case PlayerRole::KEEPER:
      return "keeper";
    case PlayerRole::ADVENTURER:
      return "adventurer";
  }
}
