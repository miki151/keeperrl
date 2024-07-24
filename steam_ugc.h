#pragma once

#include "steam_base.h"
#include "steam_info.h"

class ProgressMeter;
struct ISteamUGC;

namespace steam {

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
