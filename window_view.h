#ifndef _WINDOW_VIEW
#define _WINDOW_VIEW

#include "util.h"
#include "view.h"
#include "action.h"
#include "map_layout.h"
#include "map_gui.h"

class ViewIndex;

/** See view.h for documentation.*/
class WindowView: public View {
  public:
  
  virtual void initialize() override;
  virtual void reset() override;
  virtual void displaySplash(View::SplashType, bool& ready) override;

  virtual void close() override;

  virtual void addMessage(const string& message) override;
  virtual void addImportantMessage(const string& message) override;
  virtual void clearMessages() override;
  virtual void refreshView(const CreatureView*) override;
  virtual void updateView(const CreatureView*) override;
  virtual void drawLevelMap(const CreatureView*) override;
  virtual void resetCenter() override;
  virtual Optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index = 0,
      Optional<ActionId> exitAction = Nothing()) override;
  virtual Optional<Vec2> chooseDirection(const string& message) override;
  virtual bool yesOrNoPrompt(const string& message) override;
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) override;
  virtual void animation(Vec2 pos, AnimationId) override;

  virtual void presentText(const string& title, const string& text) override;
  virtual void presentList(const string& title, const vector<ListElem>& options, bool scrollDown = false,
      Optional<ActionId> exitAction = Nothing()) override;
  virtual Optional<int> getNumber(const string& title, int max, int increments = 1) override;

  virtual Action getAction() override;
  virtual CollectiveAction getClick(double time) override;
  virtual bool travelInterrupt() override;
  virtual int getTimeMilli() override;
  virtual void setTimeMilli(int) override;
  virtual void stopClock() override;
  virtual bool isClockStopped() override;
  virtual void continueClock() override;
  
  static Color getFireColor();

  private:

  void drawLevelMapPart(const Level* level, Rectangle levelPart, Rectangle bounds, const CreatureView* creature,
      bool drawLocations = true);
  Optional<int> chooseFromList(const string& title, const vector<ListElem>& options, int index,
      Optional<ActionId> exitAction, Optional<sf::Event::KeyEvent> exitKey, vector<sf::Event::KeyEvent> shortCuts);
  Optional<ActionId> getSimpleActionId(sf::Event::KeyEvent key);
  void refreshViewInt(const CreatureView*, bool flipBuffer = true);
  void drawMap();
  void drawPlayerInfo();
  void drawPlayerStats(GameInfo::PlayerInfo&);
  void drawBandInfo();
  void drawButtons(vector<GameInfo::BandInfo::Button> buttons, int active, vector<Rectangle>& viewButtons);
  void drawBuildings(GameInfo::BandInfo& info);
  void drawTechnology(GameInfo::BandInfo& info);
  void drawWorkshop(GameInfo::BandInfo& info);
  void drawMinions(GameInfo::BandInfo& info);
  void drawKeeperHelp();
  void drawHint(sf::Color color, const string& text);
  struct BlockingEvent {
    enum Type { IDLE, KEY, MOUSE_LEFT, MOUSE_MOVE, MOUSE_WHEEL, WHEEL_BUTTON, MINIMAP } type;
    Optional<sf::Event::KeyEvent> key;
    int wheelDiff;
  };
  BlockingEvent readkey();
  Optional<sf::Event::KeyEvent> getEventFromMenu();

  void showMessage(const string& message);
  void retireMessages();
  void drawList(const string& title, const vector<ListElem>& options, int hightlight, bool setMousePos = false);
  void refreshScreen(bool flipBuffer = true);
  void refreshText();
  void drawAndClearBuffer();
  Optional<Vec2> getHighlightedTile();

  Optional<ViewObject> drawObjectAbs(int x, int y, const ViewIndex&, int sizeX, int sizeY, Vec2 tilePos);
  void darkenObjectAbs(int x, int y);
  void clearMessageBox();
  void unzoom();
  void switchTiles();
  void resize(int, int);
  Rectangle getMapViewBounds() const;

  bool considerScrollEvent(sf::Event&);

  int messageInd = 0;
  const static unsigned int maxMsgLength = 90;
  std::deque<string> currentMessage = std::deque<string>(3, "");
  bool oldMessage = false;

  enum class CollectiveOption {
    BUILDINGS,
    MINIONS,
    TECHNOLOGY,
    WORKSHOP,
    KEY_MAPPING,
  };
  
  CollectiveOption collectiveOption = CollectiveOption::BUILDINGS;

  enum class LegendOption {
    STATS,
    OBJECTS,
  };
  
  LegendOption legendOption = LegendOption::STATS;

  MapLayout* mapLayout;
  MapGui* mapGui;

  bool gameReady = false;

  struct TileLayouts {
    MapLayout normalLayout;
    MapLayout unzoomLayout;
    bool sprites;
  };
  TileLayouts asciiLayouts;
  TileLayouts spriteLayouts;
  TileLayouts currentTileLayout;

  vector<Rectangle> bottomKeyButtons;
  vector<Rectangle> optionButtons;
  vector<Rectangle> buildingButtons;
  vector<Rectangle> workshopButtons;
  int activeBuilding = 0;
  int activeWorkshop = 0;
  vector<Rectangle> techButtons;
  vector<Rectangle> creatureGroupButtons;
  vector<Rectangle> creatureButtons;
  Optional<Rectangle> teamButton;
  Optional<Rectangle> cancelTeamButton;
  Optional<Rectangle> impButton;
  Optional<Rectangle> pauseButton;
  vector<const Creature*> uniqueCreatures;
  vector<string> creatureNames;
  string chosenCreature;
  vector<const Creature*> chosenCreatures;

  Vec2 lastMousePos;
  struct {
    double x;
    double y;
  } mouseOffset, center;
};


#endif
