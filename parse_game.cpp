#include "stdafx.h"

#include "debug.h"
#include "util.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/program_options.hpp>

#include "gzstream.h"

using namespace boost::iostreams;
using namespace boost::program_options;
using namespace boost::archive;

typedef StreamCombiner<igzstream, InputArchive> CompressedInput;

int main(int argc, char* argv[]) {
  options_description flags("Flags");
  flags.add_options()
    ("help", "Print help")
    ("input", value<string>(), "Path a KeeperRL save file")
    ("display_name", "Print display name of the save file")
    ("version", "Print version the save file");
  variables_map vars;
  store(parse_command_line(argc, argv, flags), vars);
  if (vars.count("help") || !vars.count("input")) {
    std::cout << flags << endl;
    return 0;
  }
  string path = vars["input"].as<string>();

  CompressedInput input(path.c_str());
  string name;
  int version;
  input.getArchive() >> version >> name;
  if (vars.count("display_name"))
    std::cout << name << endl;
  if (vars.count("version"))
    std::cout << version << endl;
}
