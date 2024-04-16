/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#pragma once

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/queue.hpp>
#include <cereal/types/stack.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/access.hpp>
#include <cereal/types/bitset.hpp>
#include <cereal/types/memory.hpp>
#include "extern/variant_serialize.h"
#include "serialize_optional.h"
#include "mem_usage_counter.h"

#include "stdafx.h"
#include "progress.h"

typedef cereal::BinaryInputArchive InputArchive;
typedef cereal::BinaryOutputArchive OutputArchive;

#define SUBCLASS(X) cereal::base_class<X>(this)
#define NAMED CEREAL_NVP
#define OPTION(T) make_optional_nvp(#T, T)
#define SKIP(T) make_skip_value(T)

#ifdef MEM_USAGE_TEST
#define SERIALIZABLE(T) \
  template void T::serialize(InputArchive&, unsigned); \
  template void T::serialize(OutputArchive&, unsigned); \
  template void T::serialize(MemUsageArchive&, unsigned);

#define SERIALIZABLE_TMPL(T, ...) \
  template class T<__VA_ARGS__>;\
  template void T<__VA_ARGS__>::serialize(InputArchive&, unsigned); \
  template void T<__VA_ARGS__>::serialize(OutputArchive&, unsigned); \
  template void T<__VA_ARGS__>::serialize(MemUsageArchive&, unsigned);
#else
#define SERIALIZABLE(T) \
  template void T::serialize(InputArchive&, unsigned); \
  template void T::serialize(OutputArchive&, unsigned);

#define SERIALIZABLE_TMPL(T, ...) \
  template class T<__VA_ARGS__>;\
  template void T<__VA_ARGS__>::serialize(InputArchive&, unsigned); \
  template void T<__VA_ARGS__>::serialize(OutputArchive&, unsigned);
#endif

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)

#define REGISTER_TYPE(M) static ConstructorFunction UNIQUE_NAME(Register)(\
  [] {\
  cereal::detail::OutputBindingCreator<cereal::BinaryOutputArchive, M> b(#M);\
  cereal::detail::InputBindingCreator<cereal::BinaryInputArchive, M> a(#M);\
  });

#define SERIAL(X) X

#define SERIALIZATION_DECL(A) \
  friend cereal::access; \
  A(); \
  template <class Archive> \
  void serialize(Archive& ar, const unsigned int);

#define SERIALIZATION_CONSTRUCTOR_IMPL(A) \
  A::A() {}

#define SERIALIZATION_CONSTRUCTOR_IMPL2(A, B) \
  A::B() {}

#define SERIALIZATION_CONSTRUCTOR(A) \
  A() {}

#define SERIALIZE_TMPL(CLASS, ...) \
template <class Archive> \
void CLASS::serialize(Archive& ar1, const unsigned int) { \
  ar1(__VA_ARGS__);\
}

#define SERIALIZE_DEF(CLASS, ...) \
  SERIALIZE_TMPL(CLASS, __VA_ARGS__)\
  SERIALIZABLE(CLASS);

#define SERIALIZE_ALL(...) \
  template <class Archive> \
  void serialize(Archive& ar1, const unsigned int) { \
    ar1(__VA_ARGS__); \
  }

#define SERIALIZE_ALL_NO_VERSION(...) \
  template <class Archive> \
  void serialize(Archive& ar1) { \
    ar1(__VA_ARGS__); \
  }

#define SERIALIZE_EMPTY() \
  template <class Archive> \
  void serialize(Archive&) { \
  }

template <class T, class U>
class StreamCombiner {
  public:
  template <typename ...Args>
  StreamCombiner(Args... args) : stream(args...), archive(stream) {
 //   CHECK(stream.good()) << "File not found: " << filename;
  }

  U& getArchive() {
    return archive;
  }

  T& getStream() {
    return stream;
  }

  private:
  T stream;
  U archive;
};

namespace cereal {
template <class Archive, class T>
void save(Archive& ar1, const optional<T>& elem) {
  if (elem) {
    ar1(true);
    ar1(*elem);
  } else {
    ar1(false);
  }
}

template <class Archive, class T>
void load(Archive& ar1, optional<T>& elem) {
  bool initialized;
  ar1(initialized);
  if (initialized) {
    T val;
    ar1(val);
    elem = std::move(val);
  } else
    elem = none;
  }
} // namespace cereal

template <typename T>
struct SerializeAsValue {
  T* elem;
  template <class Archive>
  void save(Archive& ar1) const {
    ar1(*elem);
  }
};

template <typename T>
SerializeAsValue<T> serializeAsValue(T* ptr) {
  return SerializeAsValue<T>{ptr};
}
