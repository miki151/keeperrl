#pragma once

#include "util.h"

struct TestStruct1 {
  int SERIAL(x);
  int SERIAL(y);
  SERIALIZE_ALL(NAMED(x), NAMED(y))
  bool operator == (const TestStruct1& o) const {
    return x == o.x && y == o.y;
  }
};

struct TestStruct2 {
  optional<TestStruct1> SERIAL(s);
  int SERIAL(v);
  SERIALIZE_ALL(NAMED(s), NAMED(v))
  bool operator == (const TestStruct2& o) const {
    return s == o.s && v == o.v;
  }
};

struct TestStruct3 {
  optional<TestStruct2> SERIAL(a);
  SERIALIZE_ALL(NAMED(a))
  bool operator == (const TestStruct3& o) const {
    return a == o.a;
  }
};
