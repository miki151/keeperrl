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
#include "dirent.h"

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
#include "clock.h"
#include "skill.h"
#include "parse_game.h"
#include "version.h"
#include "vision.h"
#include "model_builder.h"
#include "sound_library.h"
#include "audio_device.h"
#include "sokoban_input.h"
#include "keybinding_map.h"

#ifndef VSTUDIO
#include "stack_printer.h"
#endif

#ifdef VSTUDIO
#include <steam_api.h>
#include <Windows.h>
#include <dbghelp.h>
#include <tchar.h>

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
  Intervalometer meter(milliseconds{1000 / 60});
  while (!finished) {    
    while (!meter.getCount(view->getTimeMilliAbsolute())) {
    }
    view->refreshView();
  }
}

#ifdef OSX // threads have a small stack by default on OSX, and we need a larger stack here for level gen
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 4000);
  return attr;
}

static void runGame(function<void()> game, function<void()> render, bool singleThread) {
  if (singleThread)
    game();
  else {
    FATAL << "Unimplemented";
  }
}

#else
static void runGame(function<void()> game, function<void()> render, bool singleThread) {
  if (singleThread)
    game();
  else {
    thread t(render);
    game();
    t.join();
  }
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

static float getMaxVolume() {
  return 0.7;
}

static map<MusicType, float> getMaxVolumes() {
  return {{MusicType::ADV_BATTLE, 0.4}, {MusicType::ADV_PEACEFUL, 0.4}};
}

vector<pair<MusicType, string>> getMusicTracks(const string& path, bool present) {
  if (!present)
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

static int keeperMain(const variables_map&);
static options_description getOptions();

#ifdef VSTUDIO

void miniDumpFunction(unsigned int nExceptionCode, EXCEPTION_POINTERS *pException) {
  HANDLE hFile = CreateFile(_T("KeeperRL.dmp"), GENERIC_READ | GENERIC_WRITE,
    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
    MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = pException;
    mdei.ClientPointers = FALSE;
    MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(
      MiniDumpWithDataSegs |
      MiniDumpWithHandleData |
      MiniDumpWithIndirectlyReferencedMemory |
      MiniDumpWithThreadInfo |
      MiniDumpWithUnloadedModules);
    BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
      hFile, mdt, (pException != nullptr) ? &mdei : nullptr, nullptr, nullptr);
    CloseHandle(hFile);
  }
}

void miniDumpFunction3(unsigned int nExceptionCode, EXCEPTION_POINTERS *pException) {
  SteamAPI_SetMiniDumpComment("Minidump comment: SteamworksExample.exe\n");
  SteamAPI_WriteMiniDump(nExceptionCode, pException, 123);
}

LONG WINAPI miniDumpFunction2(EXCEPTION_POINTERS *ExceptionInfo) {
  miniDumpFunction(123, ExceptionInfo);
  return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  std::set_terminate(fail);
  //_set_se_translator(miniDumpFunction);
  variables_map vars;
  vector<string> args;
  try {
    args = split_winmain(lpCmdLine);
    store(command_line_parser(args).options(getOptions()).run(), vars);
  }
  catch (boost::exception& ex) {
    std::cout << "Bad command line flags.";
  }
  if (!vars.count("no_minidump"))
    SetUnhandledExceptionFilter(miniDumpFunction2);
  if (vars.count("steam")) {
    if (SteamAPI_RestartAppIfNecessary(329970))
      FATAL << "Init failure";
    if (!SteamAPI_Init()) {
      MessageBox(NULL, TEXT("Steam is not running. If you'd like to run the game without Steam, run the standalone exe binary."), TEXT("Failure"), MB_OK);
      FATAL << "Steam is not running";
    }
    std::ofstream("steam_id") << SteamUser()->GetSteamID().ConvertToUint64() << std::endl;
  }
  /*if (IsDebuggerPresent()) {
    keeperMain(vars);
  }*/

  //try {
    keeperMain(vars);
  //}
  /*catch (...) {
    return -1;
  }*/
    return 0;
}
#endif


static options_description getOptions() {
  options_description flags("KeeperRL");
  flags.add_options()
    ("help", "Print help")
    ("steam", "Run with Steam")
    ("no_minidump", "Don't write minidumps when crashed.")
    ("single_thread", "Use a single thread for rendering and game logic")
    ("user_dir", value<string>(), "Directory for options and save files")
    ("data_dir", value<string>(), "Directory containing the game data")
    ("upload_url", value<string>(), "URL for uploading maps")
    ("override_settings", value<string>(), "Override settings")
    ("run_tests", "Run all unit tests and exit")
    ("worldgen_test", value<int>(), "Test how often world generation fails")
    ("force_keeper", "Skip main menu and force keeper mode")
    ("stderr", "Log to stderr")
    ("nolog", "No logging")
    ("free_mode", "Run in free ascii mode")
#ifndef RELEASE
    ("quick_level", "")
#endif
    ("seed", value<int>(), "Use given seed")
    ("record", value<string>(), "Record game to file")
    ("replay", value<string>(), "Replay game from file");
  return flags;
}

#undef main

#ifndef VSTUDIO
#include <SDL2/SDL.h>

int main(int argc, char* argv[]) {
  StackPrinter::initialize(argv[0], time(0));
  std::set_terminate(fail);
  variables_map vars;
  store(parse_command_line(argc, argv, getOptions()), vars);
  keeperMain(vars);
}
#endif

static long long getInstallId(const string& path, RandomGen& random) {
  long long ret;
  ifstream in(path);
  if (in)
    in >> ret;
  else {
    ret = random.getLL();
    ofstream(path) << ret;
  }
  return ret;
}

const static string serverVersion = "19";

static int keeperMain(const variables_map& vars) {
  if (vars.count("help")) {
    std::cout << getOptions() << endl;
    return 0;
  }
  bool useSingleThread = true;//vars.count("single_thread");
  FatalLog.addOutput(DebugOutput::crash());
  FatalLog.addOutput(DebugOutput::toStream(std::cerr));
#ifndef RELEASE
  ogzstream compressedLog("log.gz");
  if (!vars.count("nolog"))
    InfoLog.addOutput(DebugOutput::toStream(compressedLog));
#endif
  FatalLog.addOutput(DebugOutput::toString(
      [](const string& s) { ofstream("stacktrace.out") << s << "\n" << std::flush; } ));
  if (vars.count("stderr") || vars.count("run_tests"))
    InfoLog.addOutput(DebugOutput::toStream(std::cerr));
  Skill::init();
  Technology::init();
  Spell::init();
  Vision::init();
  if (vars.count("run_tests")) {
    testAll();
    return 0;
  }
  string dataPath;
  if (vars.count("data_dir"))
    dataPath = vars["data_dir"].as<string>();
  else
    dataPath = DATA_DIR;
  string freeDataPath = dataPath + "/data_free";
  string paidDataPath = dataPath + "/data";
  string contribDataPath = dataPath + "/data_contrib";
  bool tilesPresent = !vars.count("free_mode") && !!opendir(paidDataPath.c_str());
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
  INFO << "Data path: " << dataPath;
  INFO << "User path: " << userPath;
  string uploadUrl;
  if (vars.count("upload_url"))
    uploadUrl = vars["upload_url"].as<string>();
  else
    uploadUrl = "http://keeperrl.com/~retired/" + serverVersion;
  makeDir(userPath);
  string overrideSettings;
  if (vars.count("override_settings"))
    overrideSettings = vars["override_settings"].as<string>();
  Options options(userPath + "/options.txt", overrideSettings);
  int seed = vars.count("seed") ? vars["seed"].as<int>() : int(time(0));
  Random.init(seed);
  long long installId = getInstallId(userPath + "/installId.txt", Random);
  Renderer renderer("KeeperRL", Vec2(24, 24), contribDataPath);
  FatalLog.addOutput(DebugOutput::toString([&renderer](const string& s) { renderer.showError(s);}));
  SoundLibrary* soundLibrary = nullptr;
  AudioDevice audioDevice;
  optional<string> audioError = audioDevice.initialize();
  Clock clock;
  KeybindingMap keybindingMap(userPath + "/keybindings.txt");
  GuiFactory guiFactory(renderer, &clock, &options, &keybindingMap);
  guiFactory.loadFreeImages(freeDataPath + "/images");
  if (tilesPresent) {
    guiFactory.loadNonFreeImages(paidDataPath + "/images");
    if (!audioError)
      soundLibrary = new SoundLibrary(&options, audioDevice, paidDataPath + "/sound");
  }
  if (tilesPresent)
    initializeRendererTiles(renderer, paidDataPath + "/images");
  renderer.setCursorPath(freeDataPath + "/images/mouse_cursor.png", freeDataPath + "/images/mouse_cursor2.png");
  unique_ptr<View> view;
  view.reset(WindowView::createDefaultView(
      {renderer, guiFactory, tilesPresent, &options, &clock, soundLibrary}));
#ifndef RELEASE
  InfoLog.addOutput(DebugOutput::toString([&view](const string& s) { view->logMessage(s);}));
#endif
  std::atomic<bool> gameFinished(false);
  std::atomic<bool> viewInitialized(false);
  if (useSingleThread) {
    view->initialize();
    viewInitialized = true;
  }
  if (audioError)
    view->presentText("Failed to initialize audio. The game will be started without sound.", *audioError);
  Tile::initialize(renderer, tilesPresent);
  Jukebox jukebox(&options, audioDevice, getMusicTracks(paidDataPath + "/music", tilesPresent && !audioError), getMaxVolume(), getMaxVolumes());
  FileSharing fileSharing(uploadUrl, options, installId);
  Highscores highscores(userPath + "/" + "highscores2.txt", fileSharing, &options);
  optional<GameTypeChoice> forceGame;
  if (vars.count("force_keeper"))
    forceGame = GameTypeChoice::KEEPER;
  else if (vars.count("quick_level"))
    forceGame = GameTypeChoice::QUICK_LEVEL;
  SokobanInput sokobanInput(freeDataPath + "/sokoban_input.txt", userPath + "/sokoban_state.txt");
  MainLoop loop(view.get(), &highscores, &fileSharing, freeDataPath, userPath, &options, &jukebox, &sokobanInput,
      gameFinished, useSingleThread, forceGame);
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
  auto render = [&] { renderLoop(view.get(), &options, gameFinished, viewInitialized); };
  try {
    runGame(game, render, useSingleThread);
  } catch (GameExitException ex) {
  }
  jukebox.toggle(false);
  return 0;
}

