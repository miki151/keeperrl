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

#define ProgramOptions_no_colors
#include "extern/ProgramOptions.h"

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
#include "player_role.h"
#include "campaign_type.h"
#include "dummy_view.h"

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

static void initializeRendererTiles(Renderer& r, const DirectoryPath& path) {
  r.loadTilesFromDir(path.subdirectory("orig16"), Vec2(16, 16));
//  r.loadAltTilesFromDir(path + "/orig16_scaled", Vec2(24, 24));
  r.loadTilesFromDir(path.subdirectory("orig24"), Vec2(24, 24));
//  r.loadAltTilesFromDir(path + "/orig24_scaled", Vec2(36, 36));
  r.loadTilesFromDir(path.subdirectory("orig30"), Vec2(30, 30));
//  r.loadAltTilesFromDir(path + "/orig30_scaled", Vec2(45, 45));
}

static float getMaxVolume() {
  return 0.7;
}

static map<MusicType, float> getMaxVolumes() {
  return {{MusicType::ADV_BATTLE, 0.4}, {MusicType::ADV_PEACEFUL, 0.4}};
}

vector<pair<MusicType, FilePath>> getMusicTracks(const DirectoryPath& path, bool present) {
  if (!present)
    return {};
  else
    return {
      {MusicType::INTRO, path.file("intro.ogg")},
      {MusicType::MAIN, path.file("main.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful1.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful2.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful3.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful4.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful5.ogg")},
      {MusicType::BATTLE, path.file("battle1.ogg")},
      {MusicType::BATTLE, path.file("battle2.ogg")},
      {MusicType::BATTLE, path.file("battle3.ogg")},
      {MusicType::BATTLE, path.file("battle4.ogg")},
      {MusicType::BATTLE, path.file("battle5.ogg")},
      {MusicType::NIGHT, path.file("night1.ogg")},
      {MusicType::NIGHT, path.file("night2.ogg")},
      {MusicType::NIGHT, path.file("night3.ogg")},
      {MusicType::ADV_BATTLE, path.file("adv_battle1.ogg")},
      {MusicType::ADV_BATTLE, path.file("adv_battle2.ogg")},
      {MusicType::ADV_BATTLE, path.file("adv_battle3.ogg")},
      {MusicType::ADV_BATTLE, path.file("adv_battle4.ogg")},
      {MusicType::ADV_PEACEFUL, path.file("adv_peaceful1.ogg")},
      {MusicType::ADV_PEACEFUL, path.file("adv_peaceful2.ogg")},
      {MusicType::ADV_PEACEFUL, path.file("adv_peaceful3.ogg")},
      {MusicType::ADV_PEACEFUL, path.file("adv_peaceful4.ogg")},
      {MusicType::ADV_PEACEFUL, path.file("adv_peaceful5.ogg")},
  };
}

static int keeperMain(po::parser&);
static po::parser getCommandLineFlags();

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
  catch (...) {
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


static po::parser getCommandLineFlags() {
  po::parser flags;
  flags["help"].description("Print help");
  flags["steam"].description("Run with Steam");
  flags["no_minidump"].description("Don't write minidumps when crashed.");
  flags["single_thread"].description("Do operations like loading, saving and level generation without starting an extra thread.");
  flags["user_dir"].type(po::string).description("Directory for options and save files");
  flags["data_dir"].type(po::string).description("Directory containing the game data");
  flags["upload_url"].type(po::string).description("URL for uploading maps");
  flags["restore_settings"].description("Restore settings to default values.");
  flags["run_tests"].description("Run all unit tests and exit");
  flags["worldgen_test"].type(po::i32).description("Test how often world generation fails");
  flags["worldgen_maps"].type(po::string).description("List of maps or enemy types in world generation test. Skip to test all.");
  flags["battle_level"].type(po::string).description("Path to battle test level");
  flags["battle_info"].type(po::string).description("Path to battle info file");
  flags["battle_enemy"].type(po::string).description("Battle enemy id");
  flags["endless_enemy"].type(po::string).description("Endless mode enemy index");
  flags["battle_view"].description("Open game window and display battle");
  flags["battle_rounds"].type(po::i32).description("Number of battle rounds");
  flags["stderr"].description("Log to stderr");
  flags["nolog"].description("No logging");
  flags["free_mode"].description("Run in free ascii mode");
#ifndef RELEASE
  flags["quick_game"].description("Skip main menu and load the last save file or start a single map game");
#endif
  flags["seed"].type(po::i32).description("Use given seed");
  flags["record"].type(po::string).description("Record game to file");
  flags["replay"].type(po::string).description("Replay game from file");
  return flags;
}

#undef main

#ifndef VSTUDIO
#include <SDL2/SDL.h>

int main(int argc, char* argv[]) {
#ifdef RELEASE
  StackPrinter::initialize(argv[0], time(0));
#endif
  std::set_terminate(fail);
  po::parser flags = getCommandLineFlags();
  if (!flags.parseArgs(argc, argv))
    return -1;
  keeperMain(flags);
}
#endif

static long long getInstallId(const FilePath& path, RandomGen& random) {
  long long ret;
  ifstream in(path.getPath());
  if (in)
    in >> ret;
  else {
    ret = random.getLL();
    ofstream(path.getPath()) << ret;
  }
  return ret;
}

const static string serverVersion = "22";

static int keeperMain(po::parser& commandLineFlags) {
  if (commandLineFlags["help"].was_set()) {
    std::cout << commandLineFlags << endl;
    return 0;
  }
  bool useSingleThread = commandLineFlags["single_thread"].was_set();
  FatalLog.addOutput(DebugOutput::crash());
  FatalLog.addOutput(DebugOutput::toStream(std::cerr));
  UserErrorLog.addOutput(DebugOutput::exitProgram());
  UserErrorLog.addOutput(DebugOutput::toStream(std::cerr));
#ifndef RELEASE
  ogzstream compressedLog("log.gz");
  if (!commandLineFlags["nolog"].was_set())
    InfoLog.addOutput(DebugOutput::toStream(compressedLog));
#endif
  FatalLog.addOutput(DebugOutput::toString(
      [](const string& s) { ofstream("stacktrace.out") << s << "\n" << std::flush; } ));
  if (commandLineFlags["stderr"].was_set() || commandLineFlags["run_tests"].was_set())
    InfoLog.addOutput(DebugOutput::toStream(std::cerr));
  Skill::init();
  Technology::init();
  Spell::init();
  if (commandLineFlags["run_tests"].was_set()) {
    testAll();
    return 0;
  }
  DirectoryPath dataPath([&]() -> string {
    if (commandLineFlags["data_dir"].was_set())
      return commandLineFlags["data_dir"].get().string;
    else
      return DATA_DIR;
  }());
  auto freeDataPath = dataPath.subdirectory("data_free");
  auto paidDataPath = dataPath.subdirectory("data");
  auto contribDataPath = dataPath.subdirectory("data_contrib");
  bool tilesPresent = !commandLineFlags["free_mode"].was_set() && paidDataPath.exists();
  DirectoryPath userPath([&] () -> string {
    if (commandLineFlags["user_dir"].was_set())
      return commandLineFlags["user_dir"].get().string;
#ifdef USER_DIR
    else if (const char* userDir = USER_DIR)
      return userDir;
#endif // USER_DIR
#ifndef WINDOWS
    else if (const char* localPath = std::getenv("XDG_DATA_HOME"))
      return localPath + string("/KeeperRL");
#endif
#ifdef ENABLE_LOCAL_USER_DIR // Some environments don't define XDG_DATA_HOME
    else if (const char* homePath = std::getenv("HOME"))
      return homePath + string("/.local/share/KeeperRL");
#endif // ENABLE_LOCAL_USER_DIR
    else
      return ".";
  }());
  INFO << "Data path: " << dataPath;
  INFO << "User path: " << userPath;
  string uploadUrl;
  if (commandLineFlags["upload_url"].was_set())
    uploadUrl = commandLineFlags["upload_url"].get().string;
  else
#ifdef RELEASE
    uploadUrl = "http://keeperrl.com/~retired/" + serverVersion;
#else
    uploadUrl = "http://localhost/~michal/" + serverVersion;
#endif
  //userPath.createIfDoesntExist();
  CHECK(userPath.exists()) << "User directory \"" << userPath << "\" doesn't exist.";
  auto settingsPath = userPath.file("options.txt");
  if (commandLineFlags["restore_settings"].was_set())
    remove(settingsPath.getPath());
  Options options(settingsPath);
  int seed = commandLineFlags["seed"].was_set() ? commandLineFlags["seed"].get().i32 : int(time(0));
  Random.init(seed);
  long long installId = getInstallId(userPath.file("installId.txt"), Random);
  SoundLibrary* soundLibrary = nullptr;
  AudioDevice audioDevice;
  optional<string> audioError = audioDevice.initialize();
  Clock clock;
  KeybindingMap keybindingMap(userPath.file("keybindings.txt"));
  Jukebox jukebox(
      &options,
      audioDevice,
      getMusicTracks(paidDataPath.subdirectory("music"), tilesPresent && !audioError),
      getMaxVolume(),
      getMaxVolumes());
  FileSharing fileSharing(uploadUrl, options, installId);
  Highscores highscores(userPath.file("highscores.dat"), fileSharing, &options);
  SokobanInput sokobanInput(freeDataPath.file("sokoban_input.txt"), userPath.file("sokoban_state.txt"));
  if (commandLineFlags["worldgen_test"].was_set()) {
    MainLoop loop(nullptr, &highscores, &fileSharing, freeDataPath, userPath, &options, &jukebox, &sokobanInput,
        useSingleThread);
    vector<string> types;
    if (commandLineFlags["worldgen_maps"].was_set())
      types = split(commandLineFlags["worldgen_maps"].get().string, {','});
    loop.modelGenTest(commandLineFlags["worldgen_test"].get().i32, types, Random, &options);
    return 0;
  }
  auto battleTest = [&] (View* view) {
    MainLoop loop(view, &highscores, &fileSharing, freeDataPath, userPath, &options, &jukebox, &sokobanInput,
        useSingleThread);
    auto level = commandLineFlags["battle_level"].get().string;
    auto info = commandLineFlags["battle_info"].get().string;
    auto numRounds = commandLineFlags["battle_rounds"].get().i32;
    try {
      if (commandLineFlags["endless_enemy"].was_set()) {
        auto enemy = commandLineFlags["endless_enemy"].get().string;
        optional<int> chosenEnemy;
        if (enemy != "all")
          chosenEnemy = fromString<int>(enemy);
        loop.endlessTest(numRounds, FilePath::fromFullPath(level), FilePath::fromFullPath(info), Random, chosenEnemy);
      } else {
        auto enemyId = commandLineFlags["battle_enemy"].get().string;
        loop.battleTest(numRounds, FilePath::fromFullPath(level), FilePath::fromFullPath(info), enemyId, Random);
      }
    } catch (GameExitException) {}
  };
  if (commandLineFlags["battle_level"].was_set() && !commandLineFlags["battle_view"].was_set()) {
    battleTest(new DummyView(&clock));
    return 0;
  }
  Renderer renderer(
      "KeeperRL",
      Vec2(24, 24),
      contribDataPath,
      freeDataPath.file("images/mouse_cursor.png"),
      freeDataPath.file("images/mouse_cursor2.png"));
  FatalLog.addOutput(DebugOutput::toString([&renderer](const string& s) { renderer.showError(s);}));
  UserErrorLog.addOutput(DebugOutput::toString([&renderer](const string& s) { renderer.showError(s);}));
  GuiFactory guiFactory(renderer, &clock, &options, &keybindingMap);
  guiFactory.loadFreeImages(freeDataPath.subdirectory("images"));
  if (tilesPresent) {
    guiFactory.loadNonFreeImages(paidDataPath.subdirectory("images"));
    if (!audioError)
      soundLibrary = new SoundLibrary(&options, audioDevice, paidDataPath.subdirectory("sound"));
  }
  if (tilesPresent)
    initializeRendererTiles(renderer, paidDataPath.subdirectory("images"));
  Tile::initialize(renderer, tilesPresent);
  unique_ptr<View> view;
  view.reset(WindowView::createDefaultView(
      {renderer, guiFactory, tilesPresent, &options, &clock, soundLibrary}));
#ifndef RELEASE
  InfoLog.addOutput(DebugOutput::toString([&view](const string& s) { view->logMessage(s);}));
#endif
  view->initialize();
  if (commandLineFlags["battle_level"].was_set() && commandLineFlags["battle_view"].was_set()) {
    battleTest(view.get());
    return 0;
  }
  MainLoop loop(view.get(), &highscores, &fileSharing, freeDataPath, userPath, &options, &jukebox, &sokobanInput,
      useSingleThread);
  try {
    if (audioError)
      view->presentText("Failed to initialize audio. The game will be started without sound.", *audioError);
    ofstream systemInfo(userPath.file("system_info.txt").getPath());
    systemInfo << "KeeperRL version " << BUILD_VERSION << " " << BUILD_DATE << std::endl;
    renderer.printSystemInfo(systemInfo);
    loop.start(tilesPresent, commandLineFlags["quick_game"].was_set());
  } catch (GameExitException ex) {
  }
  jukebox.toggle(false);
  return 0;
}

