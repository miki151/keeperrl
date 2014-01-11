#ifndef _WINDOW_VIEW
#define _WINDOW_VIEW

#include <SFML/Graphics.hpp>

#include "util.h"
#include "view.h"
#include "action.h"
#include "map_layout.h"

class ViewIndex;

/** See view.h for documentation.*/
class WindowView: public View {
  public:
  
  virtual void initialize() override;
  virtual void displaySplash(bool& ready) override;

  virtual void close() override;

  virtual void addMessage(const string& message) override;
  virtual void addImportantMessage(const string& message) override;
  virtual void clearMessages() override;
  virtual void refreshView(const CreatureView*) override;
  virtual void resetCenter() override;
  virtual Optional<int> chooseFromList(const string& title, const vector<string>& options, int index = 0) override;
  virtual Optional<Vec2> chooseDirection(const string& message) override;
  virtual bool yesOrNoPrompt(const string& message) override;
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) override;

  virtual void presentText(const string& title, const string& text) override;
  virtual void presentList(const string& title, const vector<string>& options, bool scrollDown) override;
  virtual Optional<int> getNumber(const string& title, int max) override;

  virtual Action getAction() override;
  virtual CollectiveAction getClick() override;
  virtual bool travelInterrupt() override;
  virtual int getTimeMilli() override;
  virtual void setTimeMilli(int) override;
  virtual void stopClock() override;
  virtual bool isClockStopped() override;
  virtual void continueClock() override;
  
  static Vec2 projectOnBorders(Rectangle area, Vec2 pos);
  private:

  void drawMap();
  void drawPlayerInfo();
  void drawBandInfo();
  void drawBuildings(GameInfo::BandInfo& info);
  void drawMinions(GameInfo::BandInfo& info);
  void drawKeeperHelp();
  struct BlockingEvent {
    enum Type { IDLE, KEY, MOUSE_LEFT, MOUSE_MOVE } type;
    Optional<sf::Event::KeyEvent> key;
  };
  BlockingEvent readkey();
  Optional<sf::Event::KeyEvent> getEventFromMenu();

  void showMessage(const string& message);
  void drawList(const string& title, const vector<string>& options, int hightlight);
  void refreshScreen(bool flipBuffer = true);
  void refreshText();
  void drawAndClearBuffer();
  Optional<Vec2> getHighlightedTile();

  void drawObjectAbs(int x, int y, const ViewIndex&, int sizeX, int sizeY, Vec2 tilePos, bool memory = false);
  void darkenObjectAbs(int x, int y);
  void clearMessageBox();
  void unzoom(bool, bool);
  void switchTiles();
  void resize(int, int);

  bool considerScrollEvent(sf::Event&);

  int messageInd = 0;
  const static unsigned int maxMsgLength = 90;
  vector<string> currentMessage = vector<string>(3, "");
  bool oldMessage = false;

  enum class CollectiveOption {
    BUILDINGS,
    MINIONS,
    LEGEND,
    KEY_MAPPING,
  };
  
  CollectiveOption collectiveOption = CollectiveOption::BUILDINGS;

  enum class LegendOption {
    OBJECTS,
    KEY_MAPPING,
  };
  
  LegendOption legendOption = LegendOption::OBJECTS;

  MapLayout* mapLayout;

  struct TileLayouts {
    MapLayout* normalLayout;
    MapLayout* unzoomLayout;
    bool sprites;
  };
  TileLayouts asciiLayouts;
  TileLayouts spriteLayouts;
  TileLayouts currentTileLayout;
  MapLayout* worldLayout;
  vector<MapLayout*> allLayouts;

  vector<Rectangle> optionButtons;
  vector<Rectangle> roomButtons;
  vector<Rectangle> creatureGroupButtons;
  vector<Rectangle> creatureButtons;
  Optional<Rectangle> descriptionButton;
  Optional<Rectangle> teamButton;
  Optional<Rectangle> cancelTeamButton;
  Optional<Rectangle> impButton;
  vector<string> creatureNames;
  string chosenCreature;
  vector<const Creature*> chosenCreatures;

  Vec2 lastMousePos;
  Vec2 mouseOffset;
  Vec2 center;
};


#endif
