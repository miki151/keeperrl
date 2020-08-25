#include "steam_internal.h"
#include "steam_friends.h"
#include "steamworks/public/steam/isteamfriends.h"
#include <map>

#define FUNC(name, ...) SteamAPI_ISteamFriends_##name

namespace steam {

struct Friends::Impl {
  intptr_t ptr;
  std::map<CSteamID, string> userNames;

  optional<string> updateUserName(CSteamID userID) {
    string name = FUNC(GetFriendPersonaName)(ptr, userID);
    if (name.empty() || name == "[unknown]")
      return none;
    userNames[userID] = name;
    return name;
  }
};

Friends::Friends(intptr_t ptr) : ptr(ptr) {
  impl->ptr = ptr;
}
Friends::~Friends() = default;

int Friends::count(unsigned flags) const {
  return FUNC(GetFriendCount)(ptr, flags);
}

vector<UserId> Friends::ids(unsigned flags) const {
  int count = this->count(flags);
  vector<UserId> out;
  out.reserve(count);
  for (int n = 0; n < count; n++)
    out.emplace_back(UserId(FUNC(GetFriendByIndex)(ptr, n, flags)));
  return out;
}

void Friends::requestUserInfo(UserId userId, bool nameOnly) {
  FUNC(RequestUserInformation)(ptr, CSteamID(userId), nameOnly);
}

optional<string> Friends::retrieveUserName(UserId userId) {
  auto it = impl->userNames.find(CSteamID(userId));
  if (it != impl->userNames.end())
    return it->second;
  return impl->updateUserName(CSteamID(userId));
}

string Friends::name() const {
  return FUNC(GetPersonaName)(ptr);
}

optional<int> Friends::retrieveUserAvatar(UserId userId, AvatarSize size) {
  int value = 0;
  CSteamID steamId(userId);
  if (size == AvatarSize::small)
    value = FUNC(GetSmallFriendAvatar)(ptr, steamId);
  else if (size == AvatarSize::medium)
    value = FUNC(GetMediumFriendAvatar)(ptr, steamId);
  else if (size == AvatarSize::large)
    value = FUNC(GetLargeFriendAvatar)(ptr, steamId);
  return value ? value : optional<int>();
}
}
#undef FUNC
