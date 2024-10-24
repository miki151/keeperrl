#pragma once

#include "steam_base.h"

#undef small
RICH_ENUM(SteamAvatarSize, small, medium, large);

struct ISteamFriends;

namespace steam {
using AvatarSize = SteamAvatarSize;

class Friends {
  STEAM_IFACE_DECL(Friends, ISteamFriends)

  static constexpr unsigned flag_all = 0xffff;

  int count(unsigned flags = flag_all) const;
  vector<UserId> ids(unsigned flags = flag_all) const;

  void requestUserInfo(UserId, bool nameOnly);
  optional<string> retrieveUserName(UserId);
  optional<int> retrieveUserAvatar(UserId, AvatarSize);

  string name() const;

  private:
  struct Impl;
  HeapAllocated<Impl> impl;
};
}
