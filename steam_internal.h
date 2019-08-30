#pragma once

#include <stdio.h>

#include "steamworks/public/steam/steam_api_common.h"
#include "steamworks/public/steam/steam_api.h"
#include "steamworks/public/steam/steam_api_flat.h"

#include <string>

namespace steam {
std::string errorText(EResult);
}
