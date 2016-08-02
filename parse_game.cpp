#include "stdafx.h"

#include "gzstream.h"

#include "debug.h"
#include "util.h"
#include "parse_game.h"

#include <boost/program_options.hpp>


using namespace boost::program_options;
using namespace boost::archive;

#ifdef PARSE_GAME

int main(int argc, char* argv[]) {
  options_description flags("Flags");
  flags.add_options()
    ("help", "Print help")
    ("input", value<string>(), "Path a KeeperRL save file")
    ("display_name", "Print display name of the save file")
    ("serial_info", "Print serialized game info of the save file")
    ("info", "Print game info of the save file")
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
  if (vars.count("serial_info")) {
    auto savedInfo = getSavedGameInfo(path);
    TextOutput output;
    output.getArchive() << *savedInfo;
    std::cout << output.getStream().str() << endl;
  }
  if (vars.count("info")) {
    auto savedInfo = getSavedGameInfo(path);
    std::cout << savedInfo->getName() << endl;
    for (auto& minion : savedInfo->getMinions())
      std::cout << EnumInfo<ViewId>::getString(minion.viewId) << " level " << minion.level << endl;
  }
}

#endif
