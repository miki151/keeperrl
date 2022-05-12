#pragma once

#include "steam_base.h"

struct ISteamUtils;

namespace steam {

class Utils {
  STEAM_IFACE_DECL(Utils, ISteamUtils);
  friend struct CallResultBase;

  // TODO: return expected ?
  pair<int, int> imageSize(int image_id) const;
  vector<unsigned char> imageData(int image_id) const;

  unsigned appId() const;
};
}
