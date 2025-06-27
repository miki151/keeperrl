#include "promotion_info.h"

template <typename Archive>
void PromotionInfo::serialize(Archive& ar, unsigned int version) {
  ar(name, viewId, applied);
  if (version >= 1)
    ar(message);
}

SERIALIZABLE(PromotionInfo)

#include "pretty_archive.h"

template <>
void PromotionInfo::serialize(PrettyInputArchive& ar, unsigned int version) {
  ar(name);
  if (ar.peek()[0] != '{')
    ar(message);
  ar(viewId, applied);
}

