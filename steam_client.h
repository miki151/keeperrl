#pragma once

#include "steam_base.h"

namespace steam {

class Client {
  public:
  Client();
  Client(const Client&) = delete;
  void operator=(const Client&) = delete;
  ~Client();

  static bool isAvailable();
  static Client& instance();

  Friends& friends();
  User& user();
  Utils& utils();
  UGC& ugc();

  // TODO: mark callback-based functions?
  optional<int> numberOfCurrentPlayers();

  string info();

  private:
  struct Impl;
  struct Ifaces;
  unique_ptr<Impl> impl;
  unique_ptr<Ifaces> ifaces;
};
}
