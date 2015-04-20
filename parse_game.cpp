#include "stdafx.h"

#include "gzstream.h"

#include "debug.h"
#include "util.h"
#include "parse_game.h"

#include <boost/program_options.hpp>


using namespace boost::program_options;
using namespace boost::archive;

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

  auto info = getNameAndVersion(path);
  if (vars.count("display_name"))
    std::cout << info->first << endl;
  if (vars.count("version"))
    std::cout << info->second << endl;
}
