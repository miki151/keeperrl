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
#include <cereal/types/boost_variant.hpp>

#include "progress.h"

typedef cereal::BinaryInputArchive InputArchive;
typedef cereal::BinaryOutputArchive OutputArchive;

#define SUBCLASS(X) cereal::base_class<X>(this)


#define SERIALIZABLE(T) \
  template void T::serialize(InputArchive&, unsigned); \
  template void T::serialize(OutputArchive&, unsigned);

#define SERIALIZABLE_TMPL(T, ...) \
  template class T<__VA_ARGS__>;\
  template void T<__VA_ARGS__>::serialize(InputArchive&, unsigned); \
  template void T<__VA_ARGS__>::serialize(OutputArchive&, unsigned);

#define REGISTER_TYPE(M) CEREAL_REGISTER_TYPE(M)

#define SERIAL(X) X
#define SVAR(X) X

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

#define SERIALIZE_DEF(CLASS, ...) \
template <class Archive> \
void CLASS::serialize(Archive& ar1, const unsigned int) { \
  ar1(__VA_ARGS__);\
}\
SERIALIZABLE(CLASS);

#define SERIALIZE_ALL(...) \
  template <class Archive> \
  void serialize(Archive& ar1, const unsigned int) { \
    ar1(__VA_ARGS__); \
  }

#define SERIALIZE_EMPTY() \
  template <class Archive> \
  void serialize(Archive&, const unsigned int) { \
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
    ar1(elem.get());
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
