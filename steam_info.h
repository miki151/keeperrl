#pragma once

// Note: while part of the steam:: api, this header can be included without USE_STEAMWORKS
// as it exclusively contains type definitions without any methods, thus not relying on
// steamworks

#include "util.h"

RICH_ENUM(SteamFindOrder, votes, date, subscriptions, playtime);
RICH_ENUM(SteamItemVisibility, public_, friends, private_);

namespace steam {

  using int64 = long long;
  using uint64 = unsigned long long;

  using FindOrder = SteamFindOrder;
  using ItemVisibility = SteamItemVisibility;

  struct UserId {
    explicit UserId(uint64 v) :value(v) {}
    operator uint64() const { return value; }

    uint64 value;
  };

  struct ItemId {
    explicit ItemId(uint64 v) :value(v) {}
    operator uint64() const { return value; }

    uint64 value;
  };

  struct DownloadInfo {
    unsigned long long bytesDownloaded;
    unsigned long long bytesTotal;
  };

  struct InstallInfo {
    unsigned long long sizeOnDisk;
    string folder;
    unsigned timeStamp;
  };

  struct ItemDetailsInfo {
    unsigned playtimeStatsDays = 0;
    bool additionalPreviews = false;
    bool children = false;
    bool keyValueTags = false;
    bool longDescription = false;
    bool metadata = false;
  };

  struct FindItemInfo {
    FindOrder order = FindOrder::playtime;
    string searchText;
    vector<string> tags;
    bool anyTag = false;

    optional<int> maxItemCount;
  };

  struct UpdateItemInfo {
    optional<ItemId> id;
    optional<string> title;
    optional<string> description;
    optional<string> folder;
    optional<string> previewFile;
    optional<ItemVisibility> visibility;
    vector<string> tags;
    optional<vector<pair<string, string>>> keyValues;
    optional<string> metadata;
  };

  struct UpdateItemResult {
    bool valid() const {
      return itemId && !error;
    }

    optional<ItemId> itemId;
    optional<string> error;
    bool requireLegalAgreement;
  };

  struct ItemStats {
    unsigned long long subscriptions, favorites, followers;
    unsigned long long uniqueSubscriptions, uniqueFavorites, uniqueFollowers;
    unsigned long long uniqueWebsiteViews, reportScore;
    unsigned long long secondsPlayed, playtimeSessions, comments;
    unsigned long long secondsPlayedDuringTimePeriod, playtimeSessionsDuringTimePeriod;
  };

  struct ItemInfo {
    ItemInfo(ItemId iid, UserId oid) :id(iid), ownerId(oid) {}

    ItemId id;
    UserId ownerId;
    std::time_t creationTime, updateTime;
    int votesUp, votesDown;
    float score;
    ItemVisibility visibility;
    bool isValid;

    string title;
    string description;
    vector<string> tags;
    heap_optional<ItemStats> stats;

    vector<pair<string, string>> keyValues;
    string metadata;
  };

}
