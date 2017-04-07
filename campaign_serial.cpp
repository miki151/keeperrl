#include "stdafx.h"
#include "campaign.h"

// Workaround for a bug in Visual Studio involving template functions

template <class Archive>
void Campaign::serialize(Archive& ar, const unsigned int version) {
  ar(sites);
  ar(playerPos, worldName, defeated, influencePos, influenceSize, playerRole, type);
}

SERIALIZABLE(Campaign);
