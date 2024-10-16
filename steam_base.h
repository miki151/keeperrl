#pragma once

#include "util.h"

#ifndef USE_STEAMWORKS
#error steam_base.h included despite USE_STEAMWORKS not being defined!
#endif


// Notes:
//
// - Whole interface is NOT thread safe, it should be used on a single thread only
//
// - Most big classes match steamworks interfaces (Client, Friends, etc.) but some
//   interfaces (like UserStats) are merged into Client class (for convenience).
//
// - Client interface is a singleton and holds all the other interfaces within itself
//   user only has to create Client to get access to all the other interfaces.
//
// - Functions with 'retrieve' prefix involve a callback and might not return information
//   on the first try (some require explicitly requesting information with other function).

// TODO: namespace
RICH_ENUM(QueryStatus, invalid, pending, completed, failed);

namespace steam {

#define STEAM_IFACE_DECL(name, Type)                                                                                         \
  Type* ptr;                                                                                                        \
  name(Type*);                                                                                                      \
  friend class Client;                                                                                                 \
                                                                                                                       \
  public:                                                                                                              \
  name(const name&) = delete;                                                                                          \
  void operator=(const name&) = delete;                                                                                \
  ~name();                                                                                                             \
                                                                                                                       \
  static name& instance();

#define STEAM_IFACE_IMPL(name, Type)                                                                                         \
  name::name(Type* ptr) : ptr(ptr) {                                                                                \
  }                                                                                                                    \
  name::~name() = default;

class Client;
class Friends;
class UGC;
class Utils;
class User;

bool initAPI();

// Also updates UGC queries
void runCallbacks();

string formatError(int value, const pair<int, const char*>* strings, int count);
string itemStateText(unsigned bits);

vector<string> parseTagList(const string&);
string formatTags(const vector<string>&);
vector<string> standardTags();

optional<int> getItemVersion(const string& metadata);

template <class CondFunc> void sleepUntil(CondFunc&& func, milliseconds duration, const atomic<bool>& cancel) {
  milliseconds timeStep(10);
  auto maxIters = duration / timeStep;
  for (int i = 0; i < maxIters; i++) {
    steam::runCallbacks();
    if (func())
      break;
    if (i + 1 < maxIters) {
      if (cancel)
        return;
      sleep_for(timeStep);
    }
  }
}

template <class T> class CallResult;
}
