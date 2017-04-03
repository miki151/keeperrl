#pragma once

#include <cereal/archives/json.hpp>

typedef cereal::JSONInputArchive TextInputArchive;
typedef cereal::JSONOutputArchive TextOutputArchive;

typedef StreamCombiner<ostringstream, TextOutputArchive> TextOutput;
typedef StreamCombiner<istringstream, TextInputArchive> TextInput;

