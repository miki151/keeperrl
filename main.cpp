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
#include "parse_game.h"
#include "vision.h"
#include "model_builder.h"
#include "sound_library.h"
#include "audio_device.h"
#include "sokoban_input.h"
#include "keybinding_map.h"
#include "campaign_type.h"
#include "dummy_view.h"
#include "sound.h"
#include "game_config.h"
#include "name_generator.h"
#include "enemy_factory.h"
#include "tileset.h"
#include "campaign_builder.h"
#include "attack_trigger.h"
#include "fx_manager.h"
#include "fx_renderer.h"
#include "fx_view_manager.h"
#include "layout_renderer.h"
#include "unlocks.h"
#include "steam_input.h"

#include "stack_printer.h"

#ifdef USE_STEAMWORKS
#include "steam_base.h"
#include "steam_client.h"
#include "steam_user.h"
#endif

#ifndef DATA_DIR
#define DATA_DIR "."
#endif

static void initializeRendererTiles(Renderer& r, const DirectoryPath& path) {
  r.setAnimationsDirectory(path.subdirectory("animations"));
  r.loadAnimations();
}

static double getMaxVolume() {
  return 0.7;
}

vector<pair<MusicType, FilePath>> getMusicTracks(const DirectoryPath& path, bool present) {
  if (!present)
    return {};
  else
    return {
      {MusicType::MAIN, path.file("main.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful1.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful2.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful3.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful4.ogg")},
      {MusicType::PEACEFUL, path.file("peaceful5.ogg")},
      {MusicType::DESERT, path.file("desert1.ogg")},
      {MusicType::DESERT, path.file("desert2.ogg")},
      {MusicType::SNOW, path.file("snow1.ogg")},
      {MusicType::SNOW, path.file("snow2.ogg")},
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

static po::parser getCommandLineFlags() {
  po::parser flags;
  flags["help"].description("Print help");
  flags["steam"].description("Run with Steam");
  flags["user_dir"].type(po::string).description("Directory for options and save files");
  flags["data_dir"].type(po::string).description("Directory containing the game data");
  flags["restore_settings"].description("Restore settings to default values.");
  flags["run_tests"].description("Run all unit tests and exit");
  flags["worldgen_test"].type(po::i32).description("Test how often world generation fails");
  flags["worldgen_maps"].type(po::string).description("List of maps or enemy types in world generation test. Skip to test all.");
  flags["battle_level"].type(po::string).description("Path to battle test level");
  flags["battle_info"].type(po::string).description("Path to battle info file");
  flags["battle_enemy"].type(po::string).description("Battle enemy id");
  flags["battle_enemy_two"].type(po::string).description("Battle enemy 2 id");
  flags["endless_enemy"].type(po::string).description("Endless mode enemy index");
  flags["battle_view"].description("Open game window and display battle");
  flags["battle_rounds"].type(po::i32).description("Number of battle rounds");
  flags["layout_size"].type(po::string).description("Size of the generated map layout");
  flags["layout_name"].type(po::string).description("Name of layout to generate");
  flags["stderr"].description("Log to stderr");
  flags["nolog"].description("No logging");
  flags["free_mode"].description("Run in free ascii mode");
  flags["gen_z_levels"].type(po::string).description("Generate and print z-level types for a given keeper");
#ifndef RELEASE
  flags["quick_game"].description("Skip main menu and load the last save file or start a single map game");
  flags["new_game"].description("Skip main menu and start a single map game");
  flags["max_turns"].type(po::i32).description("Quit the game after a given max number of turns");
#endif
  return flags;
}

#undef main

void onException() {
  if (auto ex = std::current_exception()) {
    try {
      std::rethrow_exception(ex);
    } catch (std::exception &ex) {
      FATAL << "Uncaught exception: " << ex.what();
    } catch (...) {
      FATAL << "Uncaught exception (unknown)";
    }
  } else
    FATAL << "Terminated due to unknown reason";
  // just in case FATAL doesn't crash
  abort();
}

int main(int argc, char* argv[]) {
  initializeMiniDump();
  std::set_terminate(onException);
  setInitializedStatics();
  if (argc > 1)
    attachConsole();
  po::parser flags = getCommandLineFlags();
  if (!flags.parseArgs(argc, argv))
    return -1;
  return keeperMain(flags);
}

static string getRandomInstallId(RandomGen& random) {
  string ret;
  for (int i : Range(4)) {
    ret += random.choose('e', 'u', 'i', 'o', 'a');
    ret += random.choose('q', 'w', 'r', 't', 'y', 'p', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'z', 'x', 'c', 'v', 'b',
        'n', 'm');
  }
  return ret;
}

static string getInstallId(const FilePath& path, RandomGen& random) {
  string ret;
  ifstream in(path.getPath());
  if (in)
    in >> ret;
  else {
    ret = getRandomInstallId(random);
    ofstream(path.getPath()) << ret;
  }
  return ret;
}

struct AppConfig {
  AppConfig(FilePath path) {
    if (auto error = PrettyPrinting::parseObject(values, {path}, nullptr))
      USER_FATAL << *error;
  }

  template <typename T>
  T get(const char* key) {
    if (auto value = getReferenceMaybe(values, key)) {
      if (auto ret = fromStringSafe<T>(*value))
        return *ret;
      else
        USER_FATAL << "Error reading config value: " << key << " from: " << *value;
    } else
      USER_FATAL << "Config value not found: " << key;
    fail();
  }

  private:
  map<string, string> values;
};

static void showLogoSplash(Renderer& renderer, FilePath logoPath, atomic<bool>& splashDone) {
  auto logoTexture = Texture::loadMaybe(logoPath);
  while (!splashDone) {
    renderer.drawAndClearBuffer();
    sleep_for(milliseconds(30));
    if (logoTexture) {
      auto pos = (renderer.getSize() - logoTexture->getSize()) / 2;
      renderer.drawImage(pos.x, pos.y, *logoTexture);
    }
  }
}

static int keeperMain(po::parser& commandLineFlags) {
  ENABLE_PROFILER;
  if (commandLineFlags["help"].was_set()) {
    std::cout << commandLineFlags << endl;
    return 0;
  }
  FatalLog.addOutput(DebugOutput::crash());
  FatalLog.addOutput(DebugOutput::toStream(std::cerr));
  UserErrorLog.addOutput(DebugOutput::exitProgram());
  UserErrorLog.addOutput(DebugOutput::toStream(std::cerr));
  UserInfoLog.addOutput(DebugOutput::toStream(std::cerr));
  auto trigger = AttackTrigger(StolenItems{});
  CHECK(!!trigger.getReferenceMaybe<StolenItems>());
#ifndef RELEASE
  ogzstream compressedLog("log.gz");
  /*if (!commandLineFlags["nolog"].was_set())
    InfoLog.addOutput(DebugOutput::toStream(compressedLog));*/
#endif
  FatalLog.addOutput(DebugOutput::toString(
      [](const string& s) { ofstream("stacktrace.out") << s << "\n" << std::flush; } ));
  if (commandLineFlags["stderr"].was_set() || commandLineFlags["run_tests"].was_set())
    InfoLog.addOutput(DebugOutput::toStream(std::cerr));
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
  optional<int> maxTurns;
  if (commandLineFlags["max_turns"].was_set())
    maxTurns = commandLineFlags["max_turns"].get().i32;
  Clock clock(!!maxTurns);
  userPath.createIfDoesntExist();
  auto settingsPath = userPath.file("options.txt");
  auto userKeysPath = userPath.file("keybindings.txt");
  if (commandLineFlags["restore_settings"].was_set()) {
    settingsPath.erase();
    userKeysPath.erase();
  }
  unique_ptr<MySteamInput> steamInput;
  #ifdef RELEASE
    AppConfig appConfig(dataPath.file("appconfig.txt"));
  #else
    AppConfig appConfig(dataPath.file("appconfig-dev.txt"));
  #endif
  #ifdef USE_STEAMWORKS
    steamInput = make_unique<MySteamInput>();
    optional<steam::Client> steamClient;
    if (appConfig.get<int>("steamworks") > 0) {
      if (steam::initAPI()) {
        steamClient.emplace();
        steamInput->init();
        INFO << "\n" << steamClient->info();
      }
  #ifdef RELEASE
      else
        USER_INFO << "Unable to connect with the Steam client.";
  #endif
    }
  #endif
  KeybindingMap keybindingMap(freeDataPath.file("default_keybindings.txt"), userKeysPath);
  Options options(settingsPath, &keybindingMap, steamInput.get());
  Random.init(int(time(nullptr)));
  auto installId = getInstallId(userPath.file("installId.txt"), Random);
  if (steamInput->isRunningOnDeck())
    installId += "_deck";
  SoundLibrary* soundLibrary = nullptr;
  AudioDevice audioDevice;
  optional<string> audioError = audioDevice.initialize();
  auto modsDir = userPath.subdirectory(gameConfigSubdir);
  auto allUnlocked = Unlocks::allUnlocked();
  if (commandLineFlags["gen_z_levels"].was_set()) {
    MainLoop loop(nullptr, nullptr, nullptr, paidDataPath, freeDataPath, userPath, modsDir, &options, nullptr, nullptr, nullptr,
        &allUnlocked, 0, "");
    loop.genZLevels(commandLineFlags["gen_z_levels"].get().string);
    exit(0);
  }
  if (commandLineFlags["layout_name"].was_set()) {
    USER_CHECK(commandLineFlags["layout_size"].was_set()) << "Need to specify layout_size option";
    MainLoop loop(nullptr, nullptr, nullptr, paidDataPath, freeDataPath, userPath, modsDir, &options, nullptr, nullptr, nullptr,
        &allUnlocked, 0, "");
    generateMapLayout(loop,
        commandLineFlags["layout_name"].get().string,
        freeDataPath.file("glyphs.txt"),
        commandLineFlags["layout_size"].get().string
    );
    exit(0);
  }
  SokobanInput sokobanInput(freeDataPath.file("sokoban_input.txt"), userPath.file("sokoban_state.txt"));
  string uploadUrl = appConfig.get<string>("upload_url");
  const auto modVersion = appConfig.get<string>("mod_version");
  const auto saveVersion = appConfig.get<int>("save_version");
  FileSharing fileSharing(uploadUrl, modVersion, saveVersion, options, installId);
  Highscores highscores(userPath.file("highscores.dat"));
  if (commandLineFlags["worldgen_test"].was_set()) {
    ofstream output("worldgen_out.txt");
    UserInfoLog.addOutput(DebugOutput::toStream(output));
    MainLoop loop(nullptr, &highscores, &fileSharing, paidDataPath, freeDataPath, userPath, modsDir, &options, nullptr,
        &sokobanInput, nullptr, &allUnlocked, 0, "");
    vector<string> types;
    if (commandLineFlags["worldgen_maps"].was_set())
      types = split(commandLineFlags["worldgen_maps"].get().string, {','});
    loop.modelGenTest(commandLineFlags["worldgen_test"].get().i32, types, Random, &options);
    return 0;
  }
  auto battleTest = [&] (View* view, TileSet* tileSet) {
    MainLoop loop(view, &highscores, &fileSharing, paidDataPath, freeDataPath, userPath, modsDir, &options, nullptr,
        &sokobanInput, tileSet,  &allUnlocked, 0, "");
    auto level = commandLineFlags["battle_level"].get().string;
    auto numRounds = commandLineFlags["battle_rounds"].was_set() ? commandLineFlags["battle_rounds"].get().i32 : 1;
    try {
      if (commandLineFlags["endless_enemy"].was_set()) {
        auto info = commandLineFlags["battle_info"].get().string;
        auto enemy = commandLineFlags["endless_enemy"].get().string;
        optional<int> chosenEnemy;
        if (enemy != "all")
          chosenEnemy = fromString<int>(enemy);
        loop.endlessTest(numRounds, FilePath::fromFullPath(level), FilePath::fromFullPath(info), chosenEnemy);
      } else {
        auto enemyId = commandLineFlags["battle_enemy"].get().string;
        auto enemy2Id = commandLineFlags["battle_enemy_two"].get().string;
        if (enemyId == "campaign")
          loop.campaignBattleText(numRounds, FilePath::fromFullPath(level), EnemyId(enemy2Id.data()), VillainGroup("EVIL_KEEPER"));
        else
          loop.campaignBattleText(numRounds, FilePath::fromFullPath(level), EnemyId(enemy2Id.data()), EnemyId(enemyId.data()));
      }
    } catch (GameExitException) {}
  };
  if (commandLineFlags["battle_level"].was_set() && !commandLineFlags["battle_view"].was_set()) {
    battleTest(new DummyView(&clock), nullptr);
    return 0;
  }
  Renderer renderer(
      &clock,
      steamInput.get(),
      "KeeperRL",
      contribDataPath,
      freeDataPath.file("images/mouse_cursor.png"),
      freeDataPath.file("images/mouse_cursor2.png"),
      freeDataPath.file("images/icon.png"),
      freeDataPath.file("images/map_font2.png"));
  initializeGLExtensions();

#ifndef RELEASE
  installOpenglDebugHandler();
#endif
  FatalLog.addOutput(DebugOutput::toString([&renderer](const string& s) { renderer.showError(s);}));
  UserErrorLog.addOutput(DebugOutput::toString([&renderer](const string& s) { renderer.showError(s);}));
  UserInfoLog.addOutput(DebugOutput::toString([&renderer](const string& s) { renderer.showError(s);}));
  atomic<bool> splashDone { false };
  GuiFactory guiFactory(renderer, &clock, &options, freeDataPath.subdirectory("images"));
  TileSet tileSet(paidDataPath.subdirectory("images"), modsDir, freeDataPath.subdirectory("ui"));
  renderer.setTileSet(&tileSet);
  unique_ptr<fx::FXManager> fxManager;
  unique_ptr<fx::FXRenderer> fxRenderer;
  unique_ptr<FXViewManager> fxViewManager;
  guiFactory.loadImages();
  if (tilesPresent)
    initializeRendererTiles(renderer, paidDataPath.subdirectory("images"));
  if (paidDataPath.exists()) {
    auto particlesPath = paidDataPath.subdirectory("images").subdirectory("particles");
    if (particlesPath.exists()) {
      INFO << "FX: initialization";
      fxManager = make_unique<fx::FXManager>();
      fxRenderer = make_unique<fx::FXRenderer>(particlesPath, *fxManager);
      fxRenderer->loadTextures();
      fxViewManager = make_unique<FXViewManager>(fxManager.get(), fxRenderer.get());
    }
  }
  auto loadThread = makeThread([&] {
    if (tilesPresent) {
      if (!audioError) {
        soundLibrary = new SoundLibrary(audioDevice, paidDataPath.subdirectory("sound"));
        options.addTrigger(OptionId::SOUND, [soundLibrary](int volume) {
          soundLibrary->setVolume(volume);
          soundLibrary->playSound(SoundId::SPELL_DECEPTION);
        });
        soundLibrary->setVolume(options.getIntValue(OptionId::SOUND));
      }
    }
    splashDone = true;
  });
  showLogoSplash(renderer, freeDataPath.file("images/succubi.png"), splashDone);
  loadThread.join();
  FileSharing bugreportSharing("http://retired.keeperrl.com/~bugreports", modVersion, saveVersion, options, installId);
  bugreportSharing.downloadPersonalMessage();
  unique_ptr<View> view;
  view.reset(WindowView::createDefaultView(
      {renderer, guiFactory, tilesPresent, &options, &clock, soundLibrary, &bugreportSharing, userPath, installId}));
#ifndef RELEASE
  InfoLog.addOutput(DebugOutput::toString([&view](const string& s) { view->logMessage(s);}));
#endif
  view->initialize(std::move(fxRenderer), std::move(fxViewManager));
  if (commandLineFlags["battle_level"].was_set() && commandLineFlags["battle_view"].was_set()) {
    battleTest(view.get(), &tileSet);
    return 0;
  }
  Jukebox jukebox(
      audioDevice,
      getMusicTracks(paidDataPath.subdirectory("music"), tilesPresent && !audioError),
      getMaxVolume());
  options.addTrigger(OptionId::MUSIC, [&jukebox](int volume) { jukebox.setCurrentVolume(volume); });
  Unlocks unlocks(&options, userPath.file("unlocks.txt"));
  MainLoop loop(view.get(), &highscores, &fileSharing, paidDataPath, freeDataPath, userPath, modsDir, &options, &jukebox,
      &sokobanInput, &tileSet, &unlocks, saveVersion, modVersion);
  try {
    if (audioError)
      USER_INFO << "Failed to initialize audio. The game will be started without sound. " << *audioError;
    if (commandLineFlags["quick_game"].was_set())
      loop.launchQuickGame(maxTurns, true);
    if (commandLineFlags["new_game"].was_set())
      loop.launchQuickGame(maxTurns, false);
    loop.start(tilesPresent);
  } catch (GameExitException ex) {
  }
  jukebox.toggle(false);
  return 0;
}

