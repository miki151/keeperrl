#pragma once

#include "steam_base.h"

struct ISteamUser;

namespace steam {
class User {
  STEAM_IFACE_DECL(User, ISteamUser)

  UserId id() const;
  bool isLoggedOn() const;
};
}
