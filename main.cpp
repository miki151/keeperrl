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


#include "view.h"
#include "options.h"
#include "technology.h"
#include "music.h"
#include "pantheon.h"
#include "test.h"
#include "tile.h"
#include "spell.h"
#include "window_view.h"
#include "file_sharing.h"
#include "stack_printer.h"
#include "highscores.h"
#include "main_loop.h"
#include "dirent.h"
#include "clock.h"
#include "skill.h"

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
  initialized = true;
  while (!finished) {
    view->refreshView();
  }
}

static bool tilesPresent;

#ifdef OSX // see thread comment in stdafx.h
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 2000);
  return attr;
}
#endif

void initializeRendererTiles(Renderer& r, const string& path) {
  r.loadTilesFromFile(path + "/tiles_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles2_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles3_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles4_int.png", Vec2(24, 24));
  r.loadTilesFromFile(path + "/tiles5_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles6_int.png", Vec2(36, 36));
  r.loadTilesFromFile(path + "/tiles7_int.png", Vec2(36, 36));
  r.loadTilesFromDir(path + "/shroom24", Vec2(24, 24));
  r.loadTilesFromDir(path + "/shroom36", Vec2(36, 36));
  r.loadTilesFromDir(path + "/shroom46", Vec2(46, 46));
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
    };
}
void makeDir(const string& path) {
  boost::filesystem::create_directories(path.c_str());
}

int main(int argc, char* argv[]) {
  StackPrinter::initialize(argv[0]);
  options_description flags("Flags");
  flags.add_options()
    ("help", "Print help")
    ("user_dir", value<string>(), "Directory for options and save files")
    ("data_dir", value<string>(), "Directory containing the game data")
    ("upload_url", value<string>(), "URL for uploading maps")
    ("override_settings", value<string>(), "Override settings")
    ("run_tests", "Run all unit tests and exit")
    ("gen_world_exit", "Exit after creating a world")
    ("force_keeper", "Skip main menu and force keeper mode")
    ("seed", value<int>(), "Use given seed")
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
  unique_ptr<View> view;
  unique_ptr<CompressedInput> input;
  unique_ptr<CompressedOutput> output;
  string lognamePref = "log";
  Debug::init();
  Skill::init();
  Technology::init();
  Spell::init();
  Epithet::init();
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
  else
  if (const char* localPath = std::getenv("XDG_DATA_HOME"))
    userPath = localPath + string("/KeeperRL");
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
  Renderer renderer("KeeperRL", Vec2(36, 36), contribDataPath);
  Clock clock;
  GuiFactory guiFactory(&clock);
  guiFactory.loadFreeImages(freeDataPath + "/images");
  if (tilesPresent)
    guiFactory.loadNonFreeImages(paidDataPath + "/images");
  if (tilesPresent)
    initializeRendererTiles(renderer, paidDataPath + "/images");
  int seed = vars.count("seed") ? vars["seed"].as<int>() : int(time(0));
 // int forceMode = vars.count("force_keeper") ? 0 : -1;
  bool genExit = vars.count("gen_world_exit");
  if (vars.count("replay")) {
    string fname = vars["replay"].as<string>();
    Debug() << "Reading from " << fname;
    seed = fromString<int>(fname.substr(lognamePref.size()));
    Random.init(seed);
    input.reset(new CompressedInput(fname));
    view.reset(WindowView::createReplayView(input->getArchive(),
          {renderer, guiFactory, tilesPresent, &options, &clock}));
  } else {
    Random.init(seed);
#ifndef RELEASE
    string fname(lognamePref);
    fname += toString(seed);
    output.reset(new CompressedOutput(fname));
    Debug() << "Writing to " << fname;
    view.reset(WindowView::createLoggingView(output->getArchive(),
          {renderer, guiFactory, tilesPresent, &options, &clock}));
#else
    view.reset(WindowView::createDefaultView(
          {renderer, guiFactory, tilesPresent, &options, &clock}));
#endif
  } 
  std::atomic<bool> gameFinished(false);
  std::atomic<bool> viewInitialized(false);
  Tile::initialize(renderer, tilesPresent);
  Jukebox jukebox(&options, getMusicTracks(paidDataPath + "/music"));
  FileSharing fileSharing(uploadUrl);
  Highscores highscores(userPath + "/" + "highscores.txt", fileSharing, &options);
  MainLoop loop(view.get(), &highscores, &fileSharing, freeDataPath, userPath, &options, &jukebox, gameFinished);
  auto game = [&] { while (!viewInitialized) {} loop.start(tilesPresent); };
  auto render = [&] { renderLoop(view.get(), &options, gameFinished, viewInitialized); };
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

