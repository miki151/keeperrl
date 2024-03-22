#ifdef USE_STEAMWORKS

#include "stdafx.h"
#include "steam_internal.h"
#include "steam_utils.h"
#include "steamworks/public/steam/isteamutils.h"

#define FUNC(name, ...) SteamAPI_ISteamUtils_##name

namespace steam {

Utils::Utils(ISteamUtils* ptr) : ptr(ptr) {
}
Utils::~Utils() = default;

pair<int, int> Utils::imageSize(int image_id) const {
  uint32_t w = 0, h = 0;
  if (!FUNC(GetImageSize)(ptr, image_id, &w, &h))
    CHECK(false);
  return std::make_pair(int(w), int(h));
}

vector<unsigned char> Utils::imageData(int image_id) const {
  auto size = imageSize(image_id);
  vector<unsigned char> out(size.first * size.second * 4);
  if (!FUNC(GetImageRGBA)(ptr, image_id, out.data(), out.size()))
    CHECK(false);
  return out;
}
unsigned Utils::appId() const {
  return FUNC(GetAppID)(ptr);
}
}

#endif
