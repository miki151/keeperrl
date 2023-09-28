#ifdef PARSE_GAME
#include "stdafx.h"
#include "text_serialization.h"
#include "gzstream.h"

#include "debug.h"
#include "util.h"
#include "parse_game.h"
#include "highscores.h"
#include "campaign_type.h"
#define ProgramOptions_no_colors
#include "extern/ProgramOptions.h"


const char delim = ',';

static string getString(Highscores::Score score) {
  return score.gameId + delim +
      score.playerName + delim +
      score.worldName + delim +
      score.gameResult + delim +
      ::toString<int>(score.gameWon) + delim +
      ::toString(score.points) + delim +
      ::toString(score.turns) + delim +
      EnumInfo<CampaignType>::getString(score.campaignType) + delim +
      ::toString(score.version);
}

int main(int argc, char* argv[]) {
  po::parser flags;
  setInitializedStatics();
  flags["help"].description("Print help");
  flags["input"].type(po::string).description("Path a KeeperRL save file");
  flags["display_name"].description("Print display name of the save file");
  flags["serial_info"].description("Print serialized game info of the save file");
  flags["info"].description("Print game info of the save file");
  flags["highscores"].description("Print out highscores from file");
  flags["version"].description("Print version the save file");
  if (!flags.parseArgs(argc, argv))
    return -1;
  if (flags["help"].was_set() || !flags["input"].was_set()) {
    std::cout << flags << endl;
    return 0;
  }
  FilePath inputPath = FilePath::fromFullPath(flags["input"].get().string);
  if (flags["highscores"].was_set()) {
    vector<Highscores::Score> scores;
    std::cerr << "Loading highscores from " << inputPath << std::endl;
    CompressedInput in(inputPath.getPath());
    in.getArchive() >> scores;
    for (auto& score : scores)
      std::cout << getString(score) << std::endl;
  } else {
    auto info = getNameAndVersion(inputPath);
    if (flags["display_name"].was_set())
      std::cout << info->first << endl;
    if (flags["version"].was_set())
      std::cout << info->second << endl;
    if (flags["serial_info"].was_set()) {
      auto savedInfo = loadSavedGameInfo(inputPath);
      TextOutput output;
      output.getArchive() << *savedInfo;
      std::cout << output.getStream().str() << endl;
    }
    if (flags["info"].was_set()) {
      auto savedInfo = loadSavedGameInfo(inputPath);
      std::cout << savedInfo->name << endl;
      for (auto& minion : savedInfo->minions)
        std::cout << minion.viewId.data() << " level " << minion.level << endl;
    }
  }
}

#endif
