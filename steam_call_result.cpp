#ifdef USE_STEAMWORKS

#include "steam_internal.h"
#include "steam_call_result.h"
#include "steam_utils.h"

#define FUNC(name, ...) SteamAPI_ISteamUtils_##name

namespace steam {

static const pair<int, const char*> texts[] = {{k_ESteamAPICallFailureSteamGone, "steam gone"},
                                               {k_ESteamAPICallFailureNetworkFailure, "network failure"},
                                               {k_ESteamAPICallFailureInvalidHandle, "invalid handle"},
                                               {k_ESteamAPICallFailureMismatchedCallback, "mismatched callback"}};

string errorText(ESteamAPICallFailure failure) {
  if (failure == k_ESteamAPICallFailureNone)
    return {};
  return "API call failure: " + formatError(failure, texts, arraySize(texts));
}

void CallResultBase::update(void* data, int data_size, int ident) {
  using Status = QueryStatus;
  bool failed = false;
  if (status == Status::pending) {
    auto utilsPtr = Utils::instance().ptr;
    auto is_completed = FUNC(IsAPICallCompleted)(utilsPtr, handle, &failed);
    if (!failed && is_completed) {
      auto result = FUNC(GetAPICallResult)(utilsPtr, handle, data, data_size, ident, &failed);
      if (result && !failed)
        status = Status::completed;
    }

    if (failed) {
      status = Status::failed;
      failure = FUNC(GetAPICallFailureReason)(utilsPtr, handle);
    }
  }
}

string CallResultBase::failText() const {
  return errorText(failure);
}
}

#undef FUNC

#endif
