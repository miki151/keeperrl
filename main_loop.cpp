#include "stdafx.h"
#include "main_loop.h"
#include "view.h"
#include "highscores.h"
#include "music.h"
#include "options.h"
#include "progress_meter.h"
#include "file_sharing.h"
#include "square.h"
#include "model_builder.h"
#include "parse_game.h"
#include "name_generator.h"
#include "view.h"
#include "village_control.h"
#include "campaign.h"
#include "game.h"
#include "model.h"
#include "clock.h"
#include "view_id.h"
#include "saved_game_info.h"
#include "retired_games.h"
#include "save_file_info.h"
#include "creature.h"
#include "campaign_builder.h"
#include "player_role.h"
#include "campaign_type.h"
#include "game_save_type.h"
#include "exit_info.h"
#include "tutorial.h"
#include "model.h"
#include "item_type.h"
#include "pretty_printing.h"
#include "creature_factory.h"
#include "enemy_factory.h"
#include "external_enemies.h"

MainLoop::MainLoop(View* v, Highscores* h, FileSharing* fSharing, const DirectoryPath& freePath,
    const DirectoryPath& uPath, Options* o, Jukebox* j, SokobanInput* soko, bool singleThread)
      : view(v), dataFreePath(freePath), userPath(uPath), options(o), jukebox(j),
        highscores(h), fileSharing(fSharing), useSingleThread(singleThread), sokobanInput(soko) {
}

vector<SaveFileInfo> MainLoop::getSaveFiles(const DirectoryPath& path, const string& suffix) {
  vector<SaveFileInfo> ret;
  for (auto file : path.getFiles()) {
    if (file.hasSuffix(suffix))
      ret.push_back({file.getFileName(), file.getModificationTime(), false});
  }
  sort(ret.begin(), ret.end(), [](const SaveFileInfo& a, const SaveFileInfo& b) {
        return a.date > b.date;
      });
  return ret;
}

static string getDateString(time_t t) {
  char buf[100];
  strftime(buf, sizeof(buf), "%c", std::localtime(&t));
  return buf;
}

static const int saveVersion = 2500;

static bool isCompatible(int loadedVersion) {
  return loadedVersion > 2 && loadedVersion <= saveVersion && loadedVersion / 100 == saveVersion / 100;
}

static string getSaveSuffix(GameSaveType t) {
  switch (t) {
    case GameSaveType::KEEPER: return ".kep";
    case GameSaveType::ADVENTURER: return ".adv";
    case GameSaveType::RETIRED_SITE: return ".sit";
    case GameSaveType::RETIRED_CAMPAIGN: return ".cam";
    case GameSaveType::AUTOSAVE: return ".aut";
  }
}

template <typename T>
static T loadFromFile(const FilePath& filename, bool failSilently) {
  T obj;
  try {
    CompressedInput input(filename.getPath());
    string discard;
    SavedGameInfo discard2;
    int version;
    input.getArchive() >> version >> discard >> discard2;
    input.getArchive() >> obj;
  } catch (std::exception& ex) {
    if (failSilently)
      return T();
    else
      throw ex;
  }
  return obj;
}

static bool isNotFilename(char c) {
  return !(tolower(c) >= 'a' && tolower(c) <= 'z') && !isdigit(c) && c != '_';
}

static string stripFilename(string s) {
  s.erase(remove_if(s.begin(),s.end(), isNotFilename), s.end());
  return s;
}

static void saveGame(PGame& game, const FilePath& path) {
  CompressedOutput out(path.getPath());
  string name = game->getGameDisplayName();
  SavedGameInfo savedInfo = game->getSavedGameInfo();
  out.getArchive() << saveVersion << name << savedInfo;
  out.getArchive() << game;
}

static void saveMainModel(PGame& game, const FilePath& path) {
  CompressedOutput out(path.getPath());
  string name = game->getGameDisplayName();
  SavedGameInfo savedInfo = game->getSavedGameInfo();
  out.getArchive() << saveVersion << name << savedInfo;
  out.getArchive() << game->getMainModel();
}

int MainLoop::getSaveVersion(const SaveFileInfo& save) {
  if (auto info = getNameAndVersion(userPath.file(save.filename)))
    return info->second;
  else
    return -1;
}

void MainLoop::uploadFile(const FilePath& path, GameSaveType type) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash(SplashType::BIG, "Uploading "_s + path.getPath() + "...", 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->uploadSite(path, meter);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error uploading file", *error);
}

FilePath MainLoop::getSavePath(const PGame& game, GameSaveType gameType) {
  return userPath.file(stripFilename(game->getGameIdentifier()) + getSaveSuffix(gameType));
}

const int singleModelGameSaveTime = 100000;

void MainLoop::saveUI(PGame& game, GameSaveType type, SplashType splashType) {
  auto path = getSavePath(game, type);
  if (type == GameSaveType::RETIRED_SITE) {
    int saveTime = game->getMainModel()->getSaveProgressCount();
    doWithSplash(splashType, "Retiring site...", saveTime,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveMainModel(game, path), "saving time")});
  } else {
    int saveTime = game->getSaveProgressCount();
    doWithSplash(splashType, "Saving game...", saveTime,
        [&] (ProgressMeter& meter) {
        Square::progressMeter = &meter;
        MEASURE(saveGame(game, path), "saving time")});
  }
  Square::progressMeter = nullptr;
  if (GameSaveType::RETIRED_SITE == type)
    uploadFile(path, type);
}

void MainLoop::eraseSaveFile(const PGame& game, GameSaveType type) {
  remove(getSavePath(game, type).getPath());
}

void MainLoop::getSaveOptions(const vector<pair<GameSaveType, string>>& games, vector<ListElem>& options,
    vector<SaveFileInfo>& allFiles) {
  for (auto elem : games) {
    vector<SaveFileInfo> files = getSaveFiles(userPath, getSaveSuffix(elem.first));
    files = files.filter([this] (const SaveFileInfo& info) { return isCompatible(getSaveVersion(info));});
    append(allFiles, files);
    if (!files.empty()) {
      options.emplace_back(elem.second, ListElem::TITLE);
      append(options, files.transform(
          [this] (const SaveFileInfo& info) {
              auto nameAndVersion = getNameAndVersion(userPath.file(info.filename));
              return ListElem(nameAndVersion->first, getDateString(info.date));}));
    }
  }
}

optional<SaveFileInfo> MainLoop::chooseSaveFile(const vector<ListElem>& options,
    const vector<SaveFileInfo>& allFiles, string noSaveMsg, View* view) {
  if (options.empty()) {
    view->presentText("", noSaveMsg);
    return none;
  }
  auto ind = view->chooseFromList("Choose game", options, 0);
  if (ind)
    return allFiles[*ind];
  else
    return none;
}

TimeInterval MainLoop::getAutosaveFreq() {
  return 1500_visible;
}

enum class MainLoop::ExitCondition {
  ALLIES_WON,
  ENEMIES_WON,
  TIMEOUT,
  UNKNOWN
};

MainLoop::ExitCondition MainLoop::playGame(PGame&& game, bool withMusic, bool noAutoSave,
    function<optional<ExitCondition>(WGame)> exitCondition) {
  view->reset();
  game->initialize(options, highscores, view, fileSharing);
  const milliseconds stepTimeMilli {3};
  Intervalometer meter(stepTimeMilli);
  auto lastMusicUpdate = GlobalTime(-1000);
  auto lastAutoSave = game->getGlobalTime();
  while (1) {
    double step = 1;
    if (!game->isTurnBased()) {
      double gameTimeStep = view->getGameSpeed() / stepTimeMilli.count();
      auto timeMilli = view->getTimeMilli();
      double count = meter.getCount(timeMilli);
      //INFO << "Intervalometer " << timeMilli << " " << count;
      step = min(1.0, double(count) * gameTimeStep);
    }
    INFO << "Time step " << step;
    if (auto exitInfo = game->update(step)) {
      exitInfo->visit(
          [&](ExitAndQuit) {
            eraseAllSavesExcept(game, none);
          },
          [&](GameSaveType type) {
            if (type == GameSaveType::RETIRED_SITE) {
              game->prepareSiteRetirement();
              saveUI(game, type, SplashType::BIG);
              game->doneRetirement();
            } else
              saveUI(game, type, SplashType::BIG);
            eraseAllSavesExcept(game, type);
          }
      );
      return ExitCondition::UNKNOWN;
    }
    if (exitCondition)
      if (auto c = exitCondition(game.get()))
        return *c;
    auto gameTime = game->getGlobalTime();
    if (lastMusicUpdate < gameTime && withMusic) {
      jukebox->setType(game->getCurrentMusic(), game->changeMusicNow());
      lastMusicUpdate = gameTime;
    }
    if (lastAutoSave < gameTime - getAutosaveFreq() && !noAutoSave) {
      if (options->getBoolValue(OptionId::AUTOSAVE)) {
        saveUI(game, GameSaveType::AUTOSAVE, SplashType::AUTOSAVING);
        eraseAllSavesExcept(game, GameSaveType::AUTOSAVE);
      }
      lastAutoSave = gameTime;
    }
    view->refreshView();
  }
}

void MainLoop::eraseAllSavesExcept(const PGame& game, optional<GameSaveType> except) {
  for (auto erasedType : ENUM_ALL(GameSaveType))
    if (erasedType != except)
      eraseSaveFile(game, erasedType);
}

optional<RetiredGames> MainLoop::getRetiredGames(CampaignType type) {
  switch (type) {
    case CampaignType::FREE_PLAY: {
      RetiredGames ret;
      for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_SITE)))
        if (isCompatible(getSaveVersion(info)))
          if (auto saved = getSavedGameInfo(userPath.file(info.filename)))
            ret.addLocal(*saved, info);
      optional<vector<FileSharing::SiteInfo>> onlineSites;
      doWithSplash(SplashType::SMALL, "Fetching list of retired dungeons from the server...",
          [&] { onlineSites = fileSharing->listSites(); }, [&] { fileSharing->cancel(); });
      if (onlineSites) {
        for (auto& elem : *onlineSites)
          if (isCompatible(elem.version))
            ret.addOnline(elem.gameInfo, elem.fileInfo, elem.totalGames, elem.wonGames);
      } else
        view->presentText("", "Failed to fetch list of retired dungeons from the server.");
      ret.sort();
      return ret;
    }
    case CampaignType::CAMPAIGN: {
      RetiredGames ret;
      for (auto& info : getSaveFiles(userPath, getSaveSuffix(GameSaveType::RETIRED_CAMPAIGN)))
        if (isCompatible(getSaveVersion(info)))
          if (auto saved = getSavedGameInfo(userPath.file(info.filename)))
            ret.addLocal(*saved, info);
      for (int i : All(ret.getAllGames()))
        ret.setActive(i, true);
      return ret;
    }
    default:
      return none;
  }
}

PGame MainLoop::prepareTutorial() {
  PGame game = loadGame(dataFreePath.file("tutorial.kep"));
  if (game)
    Tutorial::createTutorial(*game);
  else
    view->presentText("Sorry", "Failed to load the tutorial :(");
  return game;
}

PGame MainLoop::prepareCampaign(RandomGen& random) {
  auto choice = PlayerRoleChoice(PlayerRole::KEEPER);
  while (1) {
    choice = view->getPlayerRoleChoice(choice);
    if (auto ret = choice.match(
        [&] (PlayerRole role) -> optional<PGame> {
          CampaignBuilder builder(view, random, options, role);
          if (auto result = builder.prepareCampaign(bindMethod(&MainLoop::getRetiredGames, this), CampaignType::CAMPAIGN)) {
            return Game::campaignGame(prepareCampaignModels(*result, random), *result);
          } else
            return none;
        },
        [&] (NonRoleChoice choice) -> optional<PGame> {
          switch (choice) {
            case NonRoleChoice::LOAD_GAME:
              if (auto game = loadPrevious())
                return std::move(game);
              else
                return none;
            case NonRoleChoice::TUTORIAL:
              return prepareTutorial();
            case NonRoleChoice::GO_BACK:
              return PGame(nullptr);
          }
        }
        ))
      return std::move(*ret);
  }
}

void MainLoop::splashScreen() {
  ProgressMeter meter(1);
  jukebox->setType(MusicType::INTRO, true);
  playGame(Game::splashScreen(ModelBuilder(&meter, Random, options, sokobanInput)
        .splashModel(dataFreePath.file("splash.txt")), CampaignBuilder::getEmptyCampaign()), false, true);
}

void MainLoop::showCredits(const FilePath& path, View* view) {
  ifstream in(path.getPath());
  CHECK(!!in);
  vector<ListElem> lines;
  while (1) {
    char buf[100];
    in.getline(buf, 100);
    if (!in)
      break;
    string s(buf);
    if (s.back() == ':')
      lines.emplace_back(s, ListElem::TITLE);
    else
      lines.emplace_back(s, ListElem::NORMAL);
  }
  view->presentList("Credits", lines, false);
}

void MainLoop::playMenuMusic() {
  jukebox->setType(MusicType::MAIN, true);
}

void MainLoop::considerGameEventsPrompt() {
  if (options->getIntValue(OptionId::GAME_EVENTS) == 1) {
    if (view->yesOrNoPrompt("The imps would like to gather statistics while you're playing the game and send them anonymously to the developer. This would be very helpful in designing the game. Do you agree?"))
      options->setValue(OptionId::GAME_EVENTS, 2);
    else
      options->setValue(OptionId::GAME_EVENTS, 0);
  }
}

void MainLoop::considerFreeVersionText(bool tilesPresent) {
  if (!tilesPresent)
    view->presentText("", "You are playing a version of KeeperRL without graphical tiles. "
        "Besides lack of graphics and music, this "
        "is the same exact game as the full version. If you'd like to buy the full version, "
        "please visit keeperrl.com.\n \nYou can also get it by donating to any wildlife charity. "
        "More information on the website.");
}

void MainLoop::launchQuickGame() {
  vector<ListElem> optionsUnused;
  vector<SaveFileInfo> files;
  getSaveOptions({
      {GameSaveType::AUTOSAVE, "Recovered games:"},
      {GameSaveType::KEEPER, "Keeper games:"}}, optionsUnused, files);
  auto toLoad = std::min_element(files.begin(), files.end(),
      [](const auto& f1, const auto& f2) { return f1.date > f2.date; });
  PGame game;
  if (toLoad != files.end())
    game = loadGame(userPath.file((*toLoad).filename));
  if (!game) {
    CampaignBuilder builder(view, Random, options, PlayerRole::KEEPER);
    auto result = builder.prepareCampaign(bindMethod(&MainLoop::getRetiredGames, this), CampaignType::QUICK_MAP);
    game = Game::campaignGame(prepareCampaignModels(*result, Random), *result);
  }
  playGame(std::move(game), true, false);
}

void MainLoop::start(bool tilesPresent, bool quickGame) {
  if (options->getBoolValue(OptionId::MUSIC))
    jukebox->toggle(true);
  NameGenerator::init(dataFreePath.subdirectory("names"));
  if (quickGame)
    launchQuickGame();
  else
    splashScreen();
  view->reset();
  considerFreeVersionText(tilesPresent);
  considerGameEventsPrompt();
  int lastIndex = 0;
  while (1) {
    playMenuMusic();
    optional<int> choice;
    choice = view->chooseFromList("", {
        "Play", "Settings", "High scores", "Credits", "Quit"}, lastIndex, MenuType::MAIN);
    if (!choice)
      continue;
    lastIndex = *choice;
    switch (*choice) {
      case 0:
        if (PGame game = prepareCampaign(Random))
          playGame(std::move(game), true, false);
        view->reset();
        break;
      case 1: options->handle(view, OptionSet::GENERAL); break;
      case 2: highscores->present(view); break;
      case 3: showCredits(dataFreePath.file("credits.txt"), view); break;
      case 4: return;
    }
  }
}

#ifdef OSX // see thread comment in stdafx.h
static thread::attributes getAttributes() {
  thread::attributes attr;
  attr.set_stack_size(4096 * 4000);
  return attr;
}

static thread makeThread(function<void()> fun) {
  return thread(getAttributes(), fun);
}

#else

static thread makeThread(function<void()> fun) {
  return thread(fun);
}

#endif

void MainLoop::doWithSplash(SplashType type, const string& text, int totalProgress,
    function<void(ProgressMeter&)> fun, function<void()> cancelFun) {
  ProgressMeter meter(1.0 / totalProgress);
  if (useSingleThread)
    fun(meter);
  else {
    view->displaySplash(&meter, text, type, cancelFun);
    thread t = makeThread([fun, &meter, this] {
        try {
          fun(meter);
          view->clearSplash();
        } catch (Progress::InterruptedException) {}
      });
    try {
      view->refreshView();
      t.join();
    } catch (GameExitException e) {
      Progress::interrupt();
      t.join();
      throw e;
    }
  }
}

void MainLoop::doWithSplash(SplashType type, const string& text, function<void()> fun, function<void()> cancelFun) {
  if (useSingleThread)
    fun();
  else {
    view->displaySplash(nullptr, text, type, cancelFun);
    thread t = makeThread([fun, this] { fun(); view->clearSplash(); });
    view->refreshView();
    t.join();
  }
}

void MainLoop::modelGenTest(int numTries, const vector<string>& types, RandomGen& random, Options* options) {
  NameGenerator::init(dataFreePath.subdirectory("names"));
  ProgressMeter meter(1);
  ModelBuilder(&meter, random, options, sokobanInput).measureSiteGen(numTries, types);
}

static CreatureList readAlly(ifstream& input) {
  string ally;
  input >> ally;
  auto id = EnumInfo<CreatureId>::fromString(ally);
  CreatureList ret(1000, id);
  string levelIncreaseText;
  input >> levelIncreaseText;
  EnumMap<ExperienceType, int> levelIncrease;
  vector<ItemType> equipment;
  for (auto increase : split(levelIncreaseText, {','})) {
    int amount = fromString<int>(increase.substr(1));
    switch (increase[0]) {
      case 'M':
        levelIncrease[ExperienceType::MELEE] = amount;
        break;
      case 'S':
        levelIncrease[ExperienceType::SPELL] = amount;
        break;
      case 'A':
        levelIncrease[ExperienceType::ARCHERY] = amount;
        break;
      default:
        FATAL << "Can't parse level increase " << increase;
    }
  }
  string equipmentText;
  input >> equipmentText;
  for (auto id : split(equipmentText, {','}))
    if (auto type = PrettyPrinting::parseObject<ItemType>(id))
      equipment.push_back(*type);
    else
      FATAL << "Can't parse item type: " << id;
  ret.addInventory(equipment);
  ret.increaseBaseLevel(levelIncrease);
  return ret;
}

void MainLoop::battleTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, string enemy,
    RandomGen& random) {
  NameGenerator::init(dataFreePath.subdirectory("names"));
  ifstream input(battleInfoPath.getPath());
  int cnt = 0;
  auto enemySplit = split(enemy, {','});
  auto enemyId = EnumInfo<CreatureId>::fromString(enemySplit[0]);
  int maxEnemies = 100000;
  if (enemySplit.size() > 1)
    maxEnemies = fromString<int>(enemySplit[1]);
  input >> cnt;
  for (int i : Range(cnt)) {
    auto creatureList = readAlly(input);
    std::cout << creatureList.getSummary() << ": ";
    battleTest(numTries, levelPath, creatureList, CreatureList(maxEnemies, enemyId), random);
  }
}

void MainLoop::endlessTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath,
    RandomGen& random, optional<int> numEnemy) {
  NameGenerator::init(dataFreePath.subdirectory("names"));
  ifstream input(battleInfoPath.getPath());
  int cnt = 0;
  input >> cnt;
  vector<CreatureList> allies;
  for (int i : Range(cnt))
    allies.push_back(readAlly(input));
  ExternalEnemies enemies(random, EnemyFactory(random).getExternalEnemies());
  for (int turn : Range(100000))
    if (auto wave = enemies.popNextWave(LocalTime(turn))) {
      std::cerr << "Turn " << turn << ": " << wave->enemy.name << "\n";
      int totalWins = 0;
      for (auto& allyInfo : allies) {
        std::cerr << allyInfo.getSummary() << ": ";
        int numWins = battleTest(numTries, levelPath, allyInfo, wave->enemy.creatures, random);
        totalWins += numWins;
      }
      std::cerr << totalWins << " wins\n";
      std::cout << "Turn " << turn << ": " << wave->enemy.name << ": " << totalWins << "\n";
    }
}

int MainLoop::battleTest(int numTries, const FilePath& levelPath, CreatureList ally, CreatureList enemies,
    RandomGen& random) {
  ProgressMeter meter(1);
  int numAllies = 0;
  int numEnemies = 0;
  int numUnknown = 0;
  auto allyTribe = TribeId::getKeeper();
  std::cout.flush();
  for (int i : Range(numTries)) {
    auto game = Game::splashScreen(ModelBuilder(&meter, Random, options, sokobanInput)
        .battleModel(levelPath, ally, enemies), CampaignBuilder::getEmptyCampaign());
    auto exitCondition = [&](WGame game) -> optional<ExitCondition> {
      unordered_set<TribeId, CustomHash<TribeId>> tribes;
      for (auto& m : game->getAllModels())
        for (auto c : m->getAllCreatures())
          tribes.insert(c->getTribeId());
      if (tribes.size() == 1) {
        if (*tribes.begin() == allyTribe)
          return ExitCondition::ALLIES_WON;
        else
          return ExitCondition::ENEMIES_WON;
      }
      if (game->getGlobalTime().getVisibleInt() > 200)
        return ExitCondition::TIMEOUT;
      if (tribes.empty())
        return ExitCondition::UNKNOWN;
      else
        return none;
    };
    auto result = playGame(std::move(game), false, true, exitCondition);
    switch (result) {
      case ExitCondition::ALLIES_WON:
        ++numAllies;
        std::cerr << "a";
        break;
      case ExitCondition::ENEMIES_WON:
        ++numEnemies;
        std::cerr << "e";
        break;
      case ExitCondition::TIMEOUT:
        ++numUnknown;
        std::cerr << "t";
        break;
      case ExitCondition::UNKNOWN:
        ++numUnknown;
        std::cerr << "u";
        break;
    }
    std::cerr.flush();
  }
  std::cerr << " " << numAllies << ":" << numEnemies;
  if (numUnknown > 0)
    std::cerr << " (" << numUnknown << ") unknown";
  std::cerr << "\n";
  return numAllies;
}

PModel MainLoop::getBaseModel(ModelBuilder& modelBuilder, CampaignSetup& setup) {
  auto ret = [&] {
    switch (setup.campaign.getType()) {
      case CampaignType::SINGLE_KEEPER:
        return modelBuilder.singleMapModel(setup.campaign.getWorldName());
      case CampaignType::QUICK_MAP:
        return modelBuilder.tutorialModel("Campaign base site");
      default:
        return modelBuilder.campaignBaseModel("Campaign base site", setup.campaign.getType() == CampaignType::ENDLESS);
    }
  }();
  modelBuilder.spawnKeeper(ret.get(), std::move(setup.avatarInfo), setup.regenerateMana, setup.introMessages);
  return ret;
}

Table<PModel> MainLoop::prepareCampaignModels(CampaignSetup& setup, RandomGen& random) {
  Table<PModel> models(setup.campaign.getSites().getBounds());
  auto& sites = setup.campaign.getSites();
  for (Vec2 v : sites.getBounds())
    if (auto retired = sites[v].getRetired()) {
      if (retired->fileInfo.download)
        downloadGame(retired->fileInfo.filename);
    }
  optional<string> failedToLoad;
  NameGenerator::init(dataFreePath.subdirectory("names"));
  int numSites = setup.campaign.getNumNonEmpty();
  doWithSplash(SplashType::BIG, "Generating map...", numSites,
      [&] (ProgressMeter& meter) {
        ModelBuilder modelBuilder(nullptr, random, options, sokobanInput);
        for (Vec2 v : sites.getBounds()) {
          if (!sites[v].isEmpty())
            meter.addProgress();
          if (sites[v].getKeeper()) {
            models[v] = getBaseModel(modelBuilder, setup);
          } else if (auto villain = sites[v].getVillain())
            models[v] = modelBuilder.campaignSiteModel("Campaign enemy site", villain->enemyId, villain->type);
          else if (auto retired = sites[v].getRetired()) {
            if (PModel m = loadFromFile<PModel>(userPath.file(retired->fileInfo.filename), !useSingleThread))
              models[v] = std::move(m);
            else {
              failedToLoad = retired->fileInfo.filename;
              setup.campaign.clearSite(v);
            }
          }
        }
      });
  if (failedToLoad)
    view->presentText("Sorry", "Error reading " + *failedToLoad + ". Leaving blank site.");
  return models;
}

PGame MainLoop::loadGame(const FilePath& file) {
  PGame game;
  if (auto info = getSavedGameInfo(file))
    doWithSplash(SplashType::BIG, "Loading "_s + file.getPath() + "...", info->getProgressCount(),
        [&] (ProgressMeter& meter) {
          Square::progressMeter = &meter;
          INFO << "Loading from " << file;
          MEASURE(game = loadFromFile<PGame>(file, !useSingleThread), "Loading game");
    });
  Square::progressMeter = nullptr;
  return game;
}

bool MainLoop::downloadGame(const string& filename) {
  atomic<bool> cancelled(false);
  optional<string> error;
  doWithSplash(SplashType::BIG, "Downloading " + filename + "...", 1,
      [&] (ProgressMeter& meter) {
        error = fileSharing->download(filename, userPath, meter);
      },
      [&] {
        cancelled = true;
        fileSharing->cancel();
      });
  if (error && !cancelled)
    view->presentText("Error downloading file", *error);
  return !error;
}

static void changeSaveType(const FilePath& file, GameSaveType newType) {
  optional<FilePath> newFile;
  for (GameSaveType oldType : ENUM_ALL(GameSaveType)) {
    string suf = getSaveSuffix(oldType);
    if (file.hasSuffix(suf)) {
      if (oldType == newType)
        return;
      newFile = file.changeSuffix(suf, getSaveSuffix(newType));
      break;
    }
  }
  CHECK(!!newFile);
  remove(newFile->getPath());
  rename(file.getPath(), newFile->getPath());
}

PGame MainLoop::loadPrevious() {
  vector<ListElem> options;
  vector<SaveFileInfo> files;
  getSaveOptions({
      {GameSaveType::AUTOSAVE, "Recovered games:"},
      {GameSaveType::KEEPER, "Keeper games:"},
      {GameSaveType::ADVENTURER, "Adventurer games:"}}, options, files);
  optional<SaveFileInfo> savedGame = chooseSaveFile(options, files, "No saved games found.", view);
  if (savedGame) {
    PGame ret = loadGame(userPath.file(savedGame->filename));
    if (ret) {
      if (eraseSave())
        changeSaveType(userPath.file(savedGame->filename), GameSaveType::AUTOSAVE);
    } else
      view->presentText("Sorry", "Failed to load the save file :(");
    return ret;
  } else
    return nullptr;
}

bool MainLoop::eraseSave() {
#ifdef RELEASE
  return !options->getBoolValue(OptionId::KEEP_SAVEFILES);
#endif
  return false;
}

