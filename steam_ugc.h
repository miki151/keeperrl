#pragma once

#include "steam_base.h"

RICH_ENUM(SteamFindOrder, votes, date, subscriptions, playtime);
RICH_ENUM(SteamItemVisibility, public_, friends, private_);

class ProgressMeter;
struct ISteamUGC;

namespace steam {

using FindOrder = SteamFindOrder;
using ItemVisibility = SteamItemVisibility;

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

class UGC {
  STEAM_IFACE_DECL(UGC, ISteamUGC)

  int numSubscribedItems() const;
  vector<ItemId> subscribedItems() const;

  // TODO: return expected everywhere where something may fail ?
  // maybe just return optional?
  uint32_t itemState(ItemId) const;
  bool isDownloading(ItemId) const;
  bool isInstalled(ItemId) const;

  optional<DownloadInfo> downloadInfo(ItemId) const;
  optional<InstallInfo> installInfo(ItemId) const;
  bool downloadItem(ItemId, bool highPriority);

  using QueryId = int;
  static constexpr int maxItemsPerPage = 50;

  QueryId createDetailsQuery(const ItemDetailsInfo&, vector<ItemId>);
  QueryId createFindQuery(const FindItemInfo&, int pageId);

  void updateQueries();
  void waitForQueries(vector<QueryId>, milliseconds duration, const atomic<bool>& cancel);

  // TODO: how to report errors? use Expected<>
  bool isQueryValid(QueryId) const;
  QueryStatus queryStatus(QueryId) const;
  string queryError(QueryId, string pendingError = {}) const;

  // Results will only be returned if query is completed
  vector<ItemInfo> finishDetailsQuery(QueryId);
  vector<ItemId> finishFindQuery(QueryId);
  void finishQuery(QueryId);

  // Pass empty itemId to create new item
  void beginUpdateItem(const UpdateItemInfo&);
  optional<UpdateItemResult> tryUpdateItem(ProgressMeter&);
  bool isUpdatingItem();

  // Will remove partially created item
  void cancelUpdateItem();
  void deleteItem(ItemId);

  void startPlaytimeTracking(const vector<ItemId>&);
  void stopPlaytimeTracking(const vector<ItemId>&);

  private:
  struct QueryData;
  struct Impl;
  HeapAllocated<Impl> impl;
};
}
