#include "stdafx.h"
#include "gender.h"

const Gender Gender::male(false);
const Gender Gender::female(true);

Gender::Gender(bool f) : fem(f) {}

string Gender::his() const {
  return fem ? "her" : "his";
}

string Gender::he() const {
  return fem ? "she" : "he";
}

string Gender::god() const {
  return fem ? "goddes" : "god";
}

template <class Archive>
void Gender::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(fem);
}

SERIALIZABLE(Gender);

