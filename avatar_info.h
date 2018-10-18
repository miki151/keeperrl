#pragma once

#include "util.h"
#include "avatar_variant.h"

struct AvatarInfo {
  PCreature playerCreature;
  optional<AvatarVariant> avatarVariant;
};
