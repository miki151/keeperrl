#include "stdafx.h"
#include "campaign.h"

// Workaround for a bug in Visual Studio involving template functions

extern void serialize2(InputArchive& ar, Table<Campaign::SiteInfo>& t);
extern void serialize2(OutputArchive& ar, Table<Campaign::SiteInfo>& t);
extern void serialize2(InputArchive2& ar, Table<Campaign::SiteInfo>& t);

template <class Archive>
void Campaign::serialize(Archive& ar, const unsigned int version) {
  serialize2(ar, sites);
  //serializeAll(ar, sites);
  serializeAll(ar, playerPos, worldName, defeated, influencePos, influenceSize, playerRole, type);
}

SERIALIZABLE(Campaign);
