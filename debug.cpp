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

#include "stdafx.h"

#include "debug.h"
#include "util.h"

static void fail() {
  *((int*) 0x1234) = 0; // best way to fail
}

namespace boost {
  void assertion_failed(char const * expr, char const * function, char const * file, long line) {
    FAIL << "Assertion at " << file << ":" << (int)line;
    fail();
  }
}


Debug::Debug(DebugType t, const string& msg, int line) : type(t) {
  if (t == DebugType::FATAL)
    out = "FATAL";
  else
    out = "INFO";
  out += msg + ":" + toString(line) + " ";
}

static ofstream output;

void Debug::init(bool log) {
  if (log)
    output.open("log.out");
}

static function<void(const string&)> errorCallback;

void Debug::setErrorCallback(function<void(const string&)> error) {
  errorCallback = error;
}

void Debug::add(const string& a) {
  out += a;
}

Debug::~Debug() {
  if (type == DebugType::FATAL) {
    (ofstream("stacktrace.out") << out << endl).flush();
    if (errorCallback)
      errorCallback(out);
    fail();
  } else {
    if (output) {
      output << out << endl;
      output.flush();
    }
  }
}
Debug& Debug::operator <<(const string& msg) {
  add(msg);
  return *this;
}
Debug& Debug::operator <<(const int msg) {
  add(toString(msg));
  return *this;
}
Debug& Debug::operator <<(const long long msg) {
  add(toString(msg));
  return *this;
}
Debug& Debug::operator <<(const size_t msg) {
  add(toString(msg));
  return *this;
}
Debug& Debug::operator <<(const char msg) {
  add(toString(msg));
  return *this;
}
Debug& Debug::operator <<(const double msg) {
  add(toString(msg));
  return *this;
}

template<class T>
Debug& Debug::operator<<(const vector<T>& container){
  (*this) << "{";
  for (const T& elem : container) {
    (*this) << elem << ",";
  }
  (*this) << "}";
  return *this;
}

template<class T>
Debug& Debug::operator<<(const vector<vector<T> >& container){
  (*this) << "{";
  for (int i = 0; i < container[0].size(); ++i) {
    for (int j = 0; j < container[0].size(); ++i) {
      (*this) << container[j][i] << ",";
    }
    (*this) << '\n';
  }
  (*this) << "}";
  return *this;
}

template Debug& Debug::operator <<(const vector<string>&);
template Debug& Debug::operator <<(const vector<int>&);
template Debug& Debug::operator <<(const vector<Vec2>&);
template Debug& Debug::operator <<(const vector<double>&);
template Debug& Debug::operator <<(const vector<vector<double> >&);

template NoDebug& NoDebug::operator <<(const vector<string>&);
template NoDebug& NoDebug::operator <<(const vector<int>&);
template NoDebug& NoDebug::operator <<(const vector<Vec2>&);
template NoDebug& NoDebug::operator <<(const vector<double>&);
template NoDebug& NoDebug::operator <<(const vector<vector<double> >&);
