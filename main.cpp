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

#include <ctime>
#include <locale>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <exception>

#include "view.h"
#include "options.h"
#include "technology.h"
#include "music.h"
#include "test.h"
#include "tile.h"
#include "spell.h"
#include "window_view.h"
#include "file_sharing.h"
#include "highscores.h"
#include "main_loop.h"
#include "dirent.h"
#include "clock.h"
#include "skill.h"
#include "parse_game.h"
#include "version.h"
#include "vision.h"
#include "model_builder.h"
#include "sound_library.h"

#ifndef VSTUDIO
#include "stack_printer.h"
#endif

#ifndef DATA_DIR
#define DATA_DIR "."
#endif

#ifndef USER_DIR
#define USER_DIR "."
#endif

using namespace boost::iostreams;
using namespace boost::program_options;
using namespace boost::archive;


void renderLoop(View* view, Options* options, atomic<bool>& finished, atomic<bool>& initialized) {
  view->initialize();
  options->setChoices(OptionId::FULLSCREEN_RESOLUTION, Renderer::getFullscreenResolutions());
  initialized = true;
  while (!finished) {
    view->refreshView();
  }
}

static bool tilesPresent;

#ifdef OSX // see thread comment in stdafx.h
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 4000);
  return attr;
}
#endif

void initializeRendererTiles(Renderer& r, const string& path) {
  r.loadTilesFromDir(path + "/orig16", Vec2(16, 16));
//  r.loadAltTilesFromDir(path + "/orig16_scaled", Vec2(24, 24));
  r.loadTilesFromDir(path + "/orig24", Vec2(24, 24));
//  r.loadAltTilesFromDir(path + "/orig24_scaled", Vec2(36, 36));
  r.loadTilesFromDir(path + "/orig30", Vec2(30, 30));
//  r.loadAltTilesFromDir(path + "/orig30_scaled", Vec2(45, 45));
}

static int getMaxVolume() {
  return 70;
}

static map<MusicType, int> getMaxVolumes() {
  return {{MusicType::ADV_BATTLE, 40}, {MusicType::ADV_PEACEFUL, 40}};
}

vector<pair<MusicType, string>> getMusicTracks(const string& path) {
  if (!tilesPresent)
    return {};
  else
    return {
      {MusicType::INTRO, path + "/intro.ogg"},
      {MusicType::MAIN, path + "/main.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful1.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful2.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful3.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful4.ogg"},
      {MusicType::PEACEFUL, path + "/peaceful5.ogg"},
      {MusicType::BATTLE, path + "/battle1.ogg"},
      {MusicType::BATTLE, path + "/battle2.ogg"},
      {MusicType::BATTLE, path + "/battle3.ogg"},
      {MusicType::BATTLE, path + "/battle4.ogg"},
      {MusicType::BATTLE, path + "/battle5.ogg"},
      {MusicType::NIGHT, path + "/night1.ogg"},
      {MusicType::NIGHT, path + "/night2.ogg"},
      {MusicType::NIGHT, path + "/night3.ogg"},
      {MusicType::ADV_BATTLE, path + "/adv_battle1.ogg"},
      {MusicType::ADV_BATTLE, path + "/adv_battle2.ogg"},
      {MusicType::ADV_BATTLE, path + "/adv_battle3.ogg"},
      {MusicType::ADV_BATTLE, path + "/adv_battle4.ogg"},
      {MusicType::ADV_PEACEFUL, path + "/adv_peaceful1.ogg"},
      {MusicType::ADV_PEACEFUL, path + "/adv_peaceful2.ogg"},
      {MusicType::ADV_PEACEFUL, path + "/adv_peaceful3.ogg"},
      {MusicType::ADV_PEACEFUL, path + "/adv_peaceful4.ogg"},
      {MusicType::ADV_PEACEFUL, path + "/adv_peaceful5.ogg"},
    };
}
void makeDir(const string& path) {
  boost::filesystem::create_directories(path.c_str());
}

static void fail() {
  *((int*) 0x1234) = 0; // best way to fail
}

int main(int argc, char* argv[]) {
#ifndef VSTUDIO
  StackPrinter::initialize(argv[0], time(0));
#endif
  std::set_terminate(fail);
  options_description flags("Flags");
  flags.add_options()
    ("help", "Print help")
    ("single_thread", "Use a single thread for rendering and game logic")
    ("user_dir", value<string>(), "Directory for options and save files")
    ("data_dir", value<string>(), "Directory containing the game data")
    ("upload_url", value<string>(), "URL for uploading maps")
    ("override_settings", value<string>(), "Override settings")
    ("run_tests", "Run all unit tests and exit")
    ("worldgen_test", value<int>(), "Test how often world generation fails")
    ("force_keeper", "Skip main menu and force keeper mode")
    ("logging", "Log to log.out")
#ifndef RELEASE
    ("quick_level", "")
#endif
    ("seed", value<int>(), "Use given seed")
    ("record", value<string>(), "Record game to file")
    ("replay", value<string>(), "Replay game from file");
  variables_map vars;
  store(parse_command_line(argc, argv, flags), vars);
  if (vars.count("help")) {
    std::cout << flags << endl;
    return 0;
  }
  if (vars.count("run_tests")) {
    testAll();
    return 0;
  }
  bool useSingleThread = vars.count("single_thread");
  unique_ptr<View> view;
  unique_ptr<CompressedInput> input;
  unique_ptr<CompressedOutput> output;
  string lognamePref = "log";
  Debug::init(vars.count("logging"));
  Skill::init();
  Technology::init();
  Spell::init();
  Vision::init();
  string dataPath;
  if (vars.count("data_dir"))
    dataPath = vars["data_dir"].as<string>();
  else
    dataPath = DATA_DIR;
  string freeDataPath = dataPath + "/data_free";
  string paidDataPath = dataPath + "/data";
  string contribDataPath = dataPath + "/data_contrib";
  tilesPresent = !!opendir(paidDataPath.c_str());
  string userPath;
  if (vars.count("user_dir"))
    userPath = vars["user_dir"].as<string>();
#ifndef WINDOWS
  else
  if (const char* localPath = std::getenv("XDG_DATA_HOME"))
    userPath = localPath + string("/KeeperRL");
#endif
  else
    userPath = USER_DIR;
  Debug() << "Data path: " << dataPath;
  Debug() << "User path: " << userPath;
  string uploadUrl;
  if (vars.count("upload_url"))
    uploadUrl = vars["upload_url"].as<string>();
  else
    uploadUrl = "http://keeperrl.com/retired";
  makeDir(userPath);
  string overrideSettings;
  if (vars.count("override_settings"))
    overrideSettings = vars["override_settings"].as<string>();
  Options options(userPath + "/options.txt", overrideSettings);
  int seed = vars.count("seed") ? vars["seed"].as<int>() : int(time(0));
  Random.init(seed);
  Renderer renderer("KeeperRL", Vec2(24, 24), contribDataPath);
  SoundLibrary* soundLibrary = nullptr;
  Clock clock;
  GuiFactory guiFactory(renderer, &clock, &options);
  guiFactory.loadFreeImages(freeDataPath + "/images");
  if (tilesPresent) {
    guiFactory.loadNonFreeImages(paidDataPath + "/images");
    soundLibrary = new SoundLibrary(&options, paidDataPath + "/sound");
  }
  if (tilesPresent)
    initializeRendererTiles(renderer, paidDataPath + "/images");
  if (vars.count("replay")) {
    string fname = vars["replay"].as<string>();
    Debug() << "Reading from " << fname;
    input.reset(new CompressedInput(fname));
    input->getArchive() >> seed;
    Random.init(seed);
    view.reset(WindowView::createReplayView(input->getArchive(),
          {renderer, guiFactory, tilesPresent, &options, &clock, soundLibrary}));
  } else {
    if (vars.count("record")) {
      string fname = vars["record"].as<string>();
      output.reset(new CompressedOutput(fname));
      output->getArchive() << seed;
      Debug() << "Writing to " << fname;
      view.reset(WindowView::createLoggingView(output->getArchive(),
            {renderer, guiFactory, tilesPresent, &options, &clock, soundLibrary}));
    } else 
      view.reset(WindowView::createDefaultView(
            {renderer, guiFactory, tilesPresent, &options, &clock, soundLibrary}));
  } 
  std::atomic<bool> gameFinished(false);
  std::atomic<bool> viewInitialized(false);
  if (useSingleThread) {
    view->initialize();
    viewInitialized = true;
  }
  Tile::initialize(renderer, tilesPresent);
  Jukebox jukebox(&options, getMusicTracks(paidDataPath + "/music"), getMaxVolume(), getMaxVolumes());
  FileSharing fileSharing(uploadUrl);
  Highscores highscores(userPath + "/" + "highscores2.txt", fileSharing, &options);
  optional<GameTypeChoice> forceGame;
  if (vars.count("force_keeper"))
    forceGame = GameTypeChoice::KEEPER;
  else if (vars.count("quick_level"))
    forceGame = GameTypeChoice::QUICK_LEVEL;
  MainLoop loop(view.get(), &highscores, &fileSharing, freeDataPath, userPath, &options, &jukebox, gameFinished,
      useSingleThread, forceGame);
  if (vars.count("worldgen_test")) {
    loop.modelGenTest(vars["worldgen_test"].as<int>(), Random, &options);
    return 0;
  }
  auto game = [&] {
    while (!viewInitialized) {}
    ofstream systemInfo(userPath + "/system_info.txt");
    systemInfo << "KeeperRL version " << BUILD_VERSION << " " << BUILD_DATE << std::endl;
    renderer.printSystemInfo(systemInfo);
    loop.start(tilesPresent); };
  auto render = [&] { if (!useSingleThread) renderLoop(view.get(), &options, gameFinished, viewInitialized); };
#ifdef OSX // see thread comment in stdafx.h
  thread t(getAttributes(), game);
  render();
#else
  thread t(render);
  game();
#endif
  t.join();
  return 0;
}

