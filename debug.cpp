#include "stdafx.h"

#include "debug.h"
#include "util.h"

using namespace std;

Debug::Debug(DebugType t, const string& msg, int line) 
    : out((string[]) { "INFO ", "FATAL "}[t] + msg + ":" + convertToString(line) + " "), type(t) {
#ifdef RELEASE
  if (t == DebugType::FATAL)
    throw out;
#endif
}

static ofstream output;

void Debug::init() {
  output.open("log.out");
}

void Debug::add(const string& a) {
  out += a;
}
Debug::~Debug() {
  if (type == FATAL) {
    output << out << endl;
    output.flush();
    throw out;
  } else {
#ifndef RELEASE
    output << out << endl;
    output.flush();
#endif
  }
}
Debug& Debug::operator <<(const string& msg) {
  add(msg);
  return *this;
}
Debug& Debug::operator <<(const int msg) {
  add(convertToString(msg));
  return *this;
}
Debug& Debug::operator <<(const char msg) {
  add(convertToString(msg));
  return *this;
}
Debug& Debug::operator <<(const double msg) {
  add(convertToString(msg));
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
  for (int i : Range(container[0].size())) {
    for (int j : Range(container.size())) {
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
