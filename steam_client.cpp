#include "steam_internal.h"
#include "steam_client.h"
#include "steam_call_result.h"

#include "steam_friends.h"
#include "steam_user.h"
#include "steam_ugc.h"
#include "steam_utils.h"

#include "steamworks/public/steam/isteamclient.h"
#include "steamworks/public/steam/isteamuserstats.h"

#define FUNC(name) SteamAPI_ISteamClient_##name
#define US_FUNC(name) SteamAPI_ISteamUserStats_##name

namespace steam {

struct Client::Ifaces {
  Ifaces(ISteamClient* client, HSteamPipe pipeHandle, HSteamUser userHandle)
      : userStats(FUNC(GetISteamUserStats)(client, userHandle, pipeHandle, STEAMUSERSTATS_INTERFACE_VERSION)),
        friends(FUNC(GetISteamFriends)(client, userHandle, pipeHandle, STEAMFRIENDS_INTERFACE_VERSION)),
        user(FUNC(GetISteamUser)(client, userHandle, pipeHandle, STEAMUSER_INTERFACE_VERSION)),
        ugc(FUNC(GetISteamUGC)(client, userHandle, pipeHandle, STEAMUGC_INTERFACE_VERSION)),
        utils(FUNC(GetISteamUtils)(client, pipeHandle, STEAMUTILS_INTERFACE_VERSION)) {
  }

  ISteamUserStats* userStats;
  Friends friends;
  User user;
  UGC ugc;
  Utils utils;
};

struct Client::Impl {
  ISteamClient* client;
  HSteamPipe pipeHandle;
  HSteamUser userHandle;
  CallResult<NumberOfCurrentPlayers_t> nocp;
};

static Client* s_instance = nullptr;

bool Client::isAvailable() {
  return !!s_instance;
}

#define IFACE_INSTANCE(name, funcName)                                                                                 \
  name& name::instance() {                                                                                             \
    return Client::instance().funcName();                                                                              \
  }

IFACE_INSTANCE(Friends, friends)
IFACE_INSTANCE(User, user)
IFACE_INSTANCE(UGC, ugc)
IFACE_INSTANCE(Utils, utils)

#undef IFACE_INSTANCE

Client& Client::instance() {
  CHECK(s_instance && "Steam client not available");
  return *s_instance;
}

Client::Client() {
  CHECK(!s_instance && "At most one steam::Client class can be alive at a time");
  s_instance = this;

  // TODO: handle errors, use Expected<>
  auto client = ::SteamClient();
  auto pipeHandle = FUNC(CreateSteamPipe)(client);
  auto userHandle = FUNC(ConnectToGlobalUser)(client, pipeHandle);
  impl.reset(new Impl{client, pipeHandle, userHandle});
  ifaces.reset(new Ifaces{client, pipeHandle, userHandle});
}

Client::~Client() {
  ifaces.reset();
  FUNC(ReleaseUser)(impl->client, impl->pipeHandle, impl->userHandle);
  FUNC(BReleaseSteamPipe)(impl->client, impl->pipeHandle);
  s_instance = nullptr;
}

Friends& Client::friends() {
  return ifaces->friends;
}
User& Client::user() {
  return ifaces->user;
}
Utils& Client::utils() {
  return ifaces->utils;
}
UGC& Client::ugc() {
  return ifaces->ugc;
}

optional<int> Client::numberOfCurrentPlayers() {
  optional<int> out;
  if (!impl->nocp)
    impl->nocp = US_FUNC(GetNumberOfCurrentPlayers)(ifaces->userStats);
  else {
    impl->nocp.update();
    if (!impl->nocp.isPending()) {
      if (impl->nocp.isCompleted()) {
        out = impl->nocp.result().m_cPlayers;
      }
      // TODO: handle errors
      impl->nocp.clear();
    }
  }

  return out;
}

string Client::info() {
  char buffer[1024];
  snprintf(buffer, sizeof(buffer),
		  "STEAM: App id: %u\n"
		  "STEAM: User id: %llu\n"
		  "STEAM: User name: %s\n",
		  utils().appId(),
		  user().id().value,
		  friends().name().c_str());
  return buffer;
}
}
#undef FUNC
