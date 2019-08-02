#include "steam_internal.h"
#include "steam_user.h"
#include "steamworks/public/steam/isteamuser.h"

#define FUNC(name, ...) SteamAPI_ISteamUser_##name

namespace steam {

User::User(intptr_t ptr) : ptr(ptr) {
}
User::~User() = default;

UserId User::id() const {
  return UserId(FUNC(GetSteamID)(ptr));
}

bool User::isLoggedOn() const {
  return FUNC(BLoggedOn)(ptr);
}
}
