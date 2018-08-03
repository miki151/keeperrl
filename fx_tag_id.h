#pragma once

#include "fx_base.h"
#include <type_traits>

namespace fx {

template <class TTag, class Base /*= unsigned*/> class TagId {
public:
  using Tag = TTag;
  static_assert(std::is_unsigned<Base>::value, "");
  static_assert(sizeof(Base) <= 4, "");

  explicit TagId(int idx) : m_idx(idx) { PASSERT(idx >= 0); }

  explicit operator bool() const = delete;
  operator int() const { return m_idx; }
  int index() const { return m_idx; }

  bool operator<(TagId rhs) const { return m_idx < rhs.m_idx; }
  bool operator==(TagId rhs) const { return m_idx == rhs.m_idx; }

private:
  Base m_idx;
};
}
