#pragma once

#include "steam_base.h"

namespace steam {
class User {
  STEAM_IFACE_DECL(User)

  UserId id() const;
  bool isLoggedOn() const;
};
}
