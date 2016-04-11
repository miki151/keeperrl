#include "stdafx.h"
#include "campaign.h"

// Workaround for a bug in Visual Studio involving template functions

void serialize2(OutputArchive& ar, Table<Campaign::SiteInfo>& t) {
  ar & SVAR(t);
}
