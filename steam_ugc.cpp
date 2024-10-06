#ifdef USE_STEAMWORKS

#include "steam_internal.h"
#include "steam_ugc.h"
#include "steam_utils.h"
#include "steam_call_result.h"
#include "steamworks/public/steam/isteamugc.h"
#include "progress_meter.h"

#define FUNC(name, ...) SteamAPI_ISteamUGC_##name

namespace steam {

constexpr int UGC::maxItemsPerPage;

static_assert(sizeof(ItemId) == sizeof(PublishedFileId_t), "ItemId should be size-compatible");

using QStatus = QueryStatus;
using QHandle = UGCQueryHandle_t;
using QueryCall = CallResult<SteamUGCQueryCompleted_t>;

static const EnumMap<ItemVisibility, ERemoteStoragePublishedFileVisibility> itemVisibilityMap = {
    {ItemVisibility::public_, k_ERemoteStoragePublishedFileVisibilityPublic},
    {ItemVisibility::friends, k_ERemoteStoragePublishedFileVisibilityFriendsOnly},
    {ItemVisibility::private_, k_ERemoteStoragePublishedFileVisibilityPrivate}};

static const EnumMap<FindOrder, EUGCQuery> findOrderMap{
    {FindOrder::votes, k_EUGCQuery_RankedByVote},
    {FindOrder::date, k_EUGCQuery_RankedByPublicationDate},
    {FindOrder::subscriptions, k_EUGCQuery_RankedByTotalUniqueSubscriptions},
    {FindOrder::playtime, k_EUGCQuery_RankedByTotalPlaytime}};

struct UGC::QueryData {
  bool valid() const {
    return handle != k_UGCQueryHandleInvalid;
  }

  QHandle handle = k_UGCQueryHandleInvalid;
  optional<ItemDetailsInfo> detailsInfo;
  optional<FindItemInfo> findInfo;
  QueryCall call;
};

struct UGC::Impl {
  QueryId allocQuery(QHandle handle, unsigned long long callId) {
    int qid = -1;
    for (int n = 0; n < queries.size(); n++)
      if (!queries[n].valid()) {
        qid = n;
        break;
      }
    if (qid == -1) {
      qid = queries.size();
      queries.emplace_back();
    }

    auto& query = queries[qid];
    query.handle = handle;
    query.call = callId;
    return qid;
  }

  vector<QueryData> queries;
  optional<UpdateItemInfo> createItemInfo;
  CallResult<CreateItemResult_t> createItem;
  CallResult<SubmitItemUpdateResult_t> updateItem;
  optional<UGCUpdateHandle_t> updateHandle;
};

UGC::UGC(ISteamUGC* ptr) : ptr(ptr) {
  static_assert(maxItemsPerPage <= (int)kNumUGCResultsPerPage, "");
}
UGC::~UGC() = default;

int UGC::numSubscribedItems() const {
  return (int)FUNC(GetNumSubscribedItems)(ptr);
}

vector<ItemId> UGC::subscribedItems() const {
  vector<ItemId> out(numSubscribedItems(), ItemId(0));
  int numItems = FUNC(GetSubscribedItems)(ptr, (PublishedFileId_t*)out.data(), out.size());
  while(out.size() > numItems)
    out.pop_back();
  return out;
}

uint32_t UGC::itemState(ItemId id) const {
  return FUNC(GetItemState)(ptr, id.value);
}

bool UGC::isDownloading(ItemId id) const {
  return itemState(id) & (k_EItemStateDownloading | k_EItemStateDownloadPending);
}

bool UGC::isInstalled(ItemId id) const {
  return itemState(id) & k_EItemStateInstalled;
}

optional<DownloadInfo> UGC::downloadInfo(ItemId id) const {
  uint64 downloaded = 0, total = 0;
  if (!FUNC(GetItemDownloadInfo)(ptr, id, &downloaded, &total))
    return none;
  return DownloadInfo{downloaded, total};
}

optional<InstallInfo> UGC::installInfo(ItemId id) const {
  uint64 size_on_disk;
  uint32 time_stamp;
  char buffer[1024];
  if (!FUNC(GetItemInstallInfo)(ptr, id, &size_on_disk, buffer, sizeof(buffer) - 1, &time_stamp))
    return none;
  buffer[sizeof(buffer) - 1] = 0;
  return InstallInfo{size_on_disk, buffer, time_stamp};
}

bool UGC::downloadItem(ItemId id, bool highPriority) {
  return FUNC(DownloadItem)(ptr, id, highPriority);
}

UGC::QueryId UGC::createDetailsQuery(const ItemDetailsInfo& info, vector<ItemId> items) {
  CHECK(items.size() >= 1 && items.size() <= maxItemsPerPage) << items.size();

  auto handle = FUNC(CreateQueryUGCDetailsRequest)(ptr, (PublishedFileId_t*)items.data(), items.size());
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

#define SET_VAR(var, func)                                                                                             \
  if (info.var)                                                                                                        \
    FUNC(SetReturn##func)(ptr, handle, true);
  SET_VAR(additionalPreviews, AdditionalPreviews)
  SET_VAR(children, Children)
  SET_VAR(keyValueTags, KeyValueTags)
  SET_VAR(longDescription, LongDescription)
  SET_VAR(metadata, Metadata)
#undef SET
  if (info.playtimeStatsDays > 0)
    FUNC(SetReturnPlaytimeStats)(ptr, handle, info.playtimeStatsDays);

  auto callId = FUNC(SendQueryUGCRequest)(ptr, handle);
  auto qid = impl->allocQuery(handle, callId);
  impl->queries[qid].detailsInfo = info;
  return qid;
}

UGC::QueryId UGC::createFindQuery(const FindItemInfo& info, int pageId) {
  CHECK(pageId >= 1);
  auto appId = Utils::instance().appId();

  //auto match = k_EUGCMatchingUGCType_Items;
  auto match = k_EUGCMatchingUGCType_Items;
  auto handle = FUNC(CreateQueryAllUGCRequestPage)(ptr, findOrderMap[info.order], match, appId, appId, pageId);
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

  FUNC(SetReturnOnlyIDs)(ptr, handle, true);
  if (!info.searchText.empty())
    FUNC(SetSearchText)(ptr, handle, info.searchText.c_str());
  for (auto& tag : info.tags)
    FUNC(AddRequiredTag)(ptr, handle, tag.data());
  auto callId = FUNC(SendQueryUGCRequest)(ptr, handle);
  auto qid = impl->allocQuery(handle, callId);
  impl->queries[qid].findInfo = info;
  return qid;
}

void UGC::updateQueries() {
  for (auto& query : impl->queries)
    if (query.call.status == QStatus::pending)
      query.call.update();
}

void UGC::waitForQueries(vector<QueryId> ids, milliseconds duration, const atomic<bool>& cancel) {
  auto allFinished = [&]() {
    for (auto qid : ids)
      if (queryStatus(qid) == QStatus::pending)
        return false;
    return true;
  };
  sleepUntil(allFinished, duration, cancel);
}

bool UGC::isQueryValid(QueryId qid) const {
  return impl->queries[qid].valid();
}

QueryStatus UGC::queryStatus(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return impl->queries[qid].call.status;
}

string UGC::queryError(QueryId qid, string pendingError) const {
  CHECK(isQueryValid(qid));
  auto& call = impl->queries[qid].call;
  if (call.status == QStatus::failed)
    return call.failText();
  if (call.status == QStatus::pending)
    return pendingError;
  return "";
}

vector<ItemInfo> UGC::finishDetailsQuery(QueryId qid) {
  vector<ItemInfo> out;
  CHECK(isQueryValid(qid) && impl->queries[qid].detailsInfo);
  auto& query = impl->queries[qid];
  auto& info = impl->queries[qid].detailsInfo;

  if (query.call.status == QStatus::completed) {
    int numResults = (int)query.call.result().m_unNumResultsReturned;
    out.reserve(numResults);

    SteamUGCDetails_t details;
    for (int n = 0; n < numResults; n++) {
      auto ret = FUNC(GetQueryUGCResult)(ptr, query.handle, n, &details);
      CHECK(ret);
      ItemInfo newItem{ItemId(details.m_nPublishedFileId), UserId(details.m_ulSteamIDOwner)};
      newItem.visibility = ItemVisibility::public_;
      for (auto vis : ENUM_ALL(ItemVisibility))
        if (itemVisibilityMap[vis] == details.m_eVisibility)
          newItem.visibility = vis;
      newItem.votesUp = details.m_unVotesUp;
      newItem.votesDown = details.m_unVotesDown;
      newItem.score = details.m_flScore;
      newItem.title = details.m_rgchTitle;
      newItem.description = details.m_rgchDescription;
      newItem.tags = parseTagList(details.m_rgchTags);
      newItem.creationTime = (time_t)details.m_rtimeCreated;
      newItem.updateTime = (time_t)details.m_rtimeUpdated;
      newItem.isValid = details.m_eResult == k_EResultOK; // TODO: generate optional error string?

      constexpr int bufSize = 4096;
      if (info->keyValueTags) {
        char buf1[bufSize + 1], buf2[bufSize + 1];
        int count = (int)FUNC(GetQueryUGCNumKeyValueTags)(ptr, query.handle, n);

        newItem.keyValues.resize(count);
        for (int i = 0; i < count; i++) {
          auto ret = FUNC(GetQueryUGCKeyValueTag)(ptr, query.handle, n, i, buf1, bufSize, buf2, bufSize);
          CHECK(ret);
          buf1[bufSize] = buf2[bufSize] = 0;
          newItem.keyValues[n] = make_pair(buf1, buf2);
        }
      }
      if (info->metadata) {
        char buffer[bufSize + 1];
        auto result = FUNC(GetQueryUGCMetadata)(ptr, query.handle, n, buffer, bufSize);
        buffer[bufSize] = 0;
        CHECK(result);
        newItem.metadata = buffer;
      }
      if (info->playtimeStatsDays) {
        ItemStats stats;
        memset(&stats, 0, sizeof(stats));

#define STAT(name, enum_name)                                                                                          \
  FUNC(GetQueryUGCStatistic)(ptr, query.handle, n, k_EItemStatistic_##enum_name, &stats.name);
        STAT(subscriptions, NumSubscriptions)
        STAT(favorites, NumFavorites)
        STAT(followers, NumFollowers)
        STAT(uniqueSubscriptions, NumUniqueSubscriptions)
        STAT(uniqueFavorites, NumUniqueFavorites)
        STAT(uniqueFollowers, NumUniqueFollowers)
        STAT(uniqueWebsiteViews, NumUniqueWebsiteViews)
        STAT(reportScore, ReportScore)
        STAT(secondsPlayed, NumSecondsPlayed)
        STAT(playtimeSessions, NumPlaytimeSessions)
        STAT(comments, NumComments)
        STAT(secondsPlayedDuringTimePeriod, NumSecondsPlayedDuringTimePeriod)
        STAT(playtimeSessionsDuringTimePeriod, NumPlaytimeSessionsDuringTimePeriod)
#undef STAT
        newItem.stats = stats;
      }

      out.emplace_back(newItem);
    }
  }

  FUNC(ReleaseQueryUGCRequest)(ptr, query.handle);
  query.handle = k_UGCQueryHandleInvalid;
  return out;
}

vector<ItemId> UGC::finishFindQuery(QueryId qid) {
  vector<ItemId> out;
  CHECK(isQueryValid(qid) && impl->queries[qid].findInfo);
  auto& query = impl->queries[qid];

  if (query.call.status == QStatus::completed) {
    int numResults = (int)query.call.result().m_unNumResultsReturned;
    out.reserve(numResults);

    SteamUGCDetails_t details;
    for (int n = 0; n < numResults; n++) {
      auto ret = FUNC(GetQueryUGCResult)(ptr, query.handle, n, &details);
      CHECK(ret);
      out.emplace_back(details.m_nPublishedFileId);
    }
  }

  FUNC(ReleaseQueryUGCRequest)(ptr, query.handle);
  query.handle = k_UGCQueryHandleInvalid;
  return out;
}

void UGC::finishQuery(QueryId qid) {
  CHECK(isQueryValid(qid));
  auto& query = impl->queries[qid];
  FUNC(ReleaseQueryUGCRequest)(ptr, query.handle);
  query.handle = k_UGCQueryHandleInvalid;
}

void UGC::beginUpdateItem(const UpdateItemInfo& info) {
  CHECK(!isUpdatingItem());
  auto appId = Utils::instance().appId();

  if (info.id) {
    auto handle = FUNC(StartItemUpdate)(ptr, appId, *info.id);
    impl->updateHandle = handle;

    if (info.title)
      FUNC(SetItemTitle)(ptr, handle, info.title->c_str());
    if (info.description)
      FUNC(SetItemDescription)(ptr, handle, info.description->c_str());
    if (info.folder)
      FUNC(SetItemContent)(ptr, handle, info.folder->c_str());
    if (info.previewFile) {
      FUNC(SetItemPreview)(ptr, handle, info.previewFile->c_str());
    }
    if (info.visibility)
      FUNC(SetItemVisibility)(ptr, handle, itemVisibilityMap[*info.visibility]);
    if (!info.tags.empty()) {
      vector<const char*> buffer;
      for (auto& tag : info.tags)
        buffer.push_back(tag.c_str());
      SteamParamStringArray_t strings;
      strings.m_nNumStrings = buffer.size();
      strings.m_ppStrings = buffer.data();
      auto ret = SteamAPI_ISteamUGC_SetItemTags(ptr, handle, &strings, true);
      CHECK(ret);
    }

    if (info.keyValues)
      for (auto& pair : *info.keyValues) {
        auto ret = FUNC(AddItemKeyValueTag)(ptr, handle, pair.first.c_str(), pair.second.c_str());
        CHECK(ret);
      }
    if (info.metadata) {
      auto ret = FUNC(SetItemMetadata)(ptr, handle, info.metadata->c_str());
      CHECK(ret);
    }

    impl->updateItem = FUNC(SubmitItemUpdate)(ptr, handle, nullptr);
  } else {
    impl->createItem = FUNC(CreateItem)(ptr, appId, k_EWorkshopFileTypeCommunity);
    impl->createItemInfo = info;
  }
}

optional<UpdateItemResult> UGC::tryUpdateItem(ProgressMeter& meter) {
  if (impl->createItem) {
    impl->createItem.update();
    if (impl->createItem.isCompleted()) {
      auto& out = impl->createItem.result();
      impl->createItem.clear();

      if (out.m_eResult == k_EResultOK) {
        impl->createItemInfo->id = ItemId(out.m_nPublishedFileId);
        beginUpdateItem(*impl->createItemInfo);
        return none;
      } else {
        auto error = string("Error while creating item: ") + errorText(out.m_eResult);
        return UpdateItemResult{none, error, false};
      }
    }
    return none;
  }

  impl->updateItem.update();
  if (impl->updateHandle) {
    uint64 downloaded = 0, total = 0;
    auto res = FUNC(GetItemUpdateProgress)(ptr, *impl->updateHandle, &downloaded, &total);
    std::cout << "progress " << res << " " << downloaded << ", " << total << std::endl;
    meter.setProgress(float(res) / 5);
  }
  if (impl->updateItem.isCompleted()) {
    auto& out = impl->updateItem.result();
    impl->updateItem.clear();
    impl->createItemInfo = none;
    optional<string> error;
    if (out.m_eResult != k_EResultOK)
      error = string("Error while updating item: ") + errorText(out.m_eResult);
    return UpdateItemResult{ItemId(out.m_nPublishedFileId), error, out.m_bUserNeedsToAcceptWorkshopLegalAgreement};
  }
  return none;
}

bool UGC::isUpdatingItem() {
  return !!impl->createItem || !!impl->updateItem;
}

void UGC::cancelUpdateItem() {
  auto& itemInfo = impl->createItemInfo;
  if (itemInfo && itemInfo->id && impl->updateItem)
    deleteItem(*itemInfo->id);
  impl->createItem.clear();
  itemInfo = none;
  impl->updateItem.clear();
}

void UGC::deleteItem(ItemId id) {
  FUNC(DeleteItem)(ptr, id);
}

void UGC::startPlaytimeTracking(const vector<ItemId>& ids) {
  INFO << "STEAM: playtime tracking started for: " << ids;
  FUNC(StartPlaytimeTracking)(ptr, (PublishedFileId_t*)ids.data(), (unsigned)ids.size());
}

void UGC::stopPlaytimeTracking(const vector<ItemId>& ids) {
  INFO << "STEAM: playtime tracking stopped for: " << ids;
  FUNC(StopPlaytimeTracking)(ptr, (PublishedFileId_t*)ids.data(), (unsigned)ids.size());
}
}
#undef FUNC

#endif
