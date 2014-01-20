#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "map_memory.h"
#include "view.h"
#include "monster_ai.h"
#include "creature_view.h"
#include "creature_factory.h"
#include "markov_chain.h"
#include "minion_equipment.h"


class Collective : public CreatureView, public EventListener {
  public:
  Collective(CreatureFactory minionFactory, CreatureFactory undeadFactory);
  virtual const MapMemory& getMemory(const Level* l) const override;
  virtual ViewIndex getViewIndex(Vec2 pos) const override;
  virtual void refreshGameInfo(View::GameInfo&) const  override;
  virtual Vec2 getPosition() const  override;
  virtual bool canSee(const Creature*) const  override;
  virtual bool canSee(Vec2 position) const  override;
  virtual vector<const Creature*> getUnknownAttacker() const override;

  virtual bool staticPosition() const override;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;
  virtual void onTriggerEvent(const Level*, Vec2 pos) override;
  virtual void onSquareReplacedEvent(const Level*, Vec2 pos) override;
  virtual void onChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) override;

  void processInput(View* view);
  void tick();
  void update(Creature*);
  MoveInfo getMove(Creature* c);
  MoveInfo getMinionMove(Creature* c);
  void addCreature(Creature* c);
  void setLevel(Level* l);

  virtual const Level* getLevel() const;

  void onConstructed(Vec2 pos, SquareType);
  void onBrought(Vec2 pos, vector<Item*> items);
  void onAppliedItem(Vec2 pos, Item* item);
  void onAppliedItemCancel(Vec2 pos);
  void onPickedUp(Vec2 pos, vector<Item*> items);

  Vec2 getHeartPos() const;
  void render(View*);
  void possess(const Creature*, View*);

  bool isTurnBased();

  enum class ResourceId {
    GOLD,
    WOOD,
    IRON,
  };

  struct ResourceInfo {
    SquareType storageType;
    ItemPredicate predicate;
    ItemId itemId;
  };


  private:
  struct BuildInfo {
    struct SquareInfo {
      SquareType type;
      ResourceId resourceId;
      int cost;
      string name;
    } squareInfo;

    struct TrapInfo {
      TrapType type;
      string name;
      ViewId viewId;
    } trapInfo;

    enum BuildType { SQUARE, CUT_TREE, IMP, TRAP, GUARD_POST} buildType;

    BuildInfo(SquareInfo info) : squareInfo(info), buildType(SQUARE) {}
    BuildInfo(TrapInfo info) : trapInfo(info), buildType(TRAP) {}
    BuildInfo(BuildType type) : buildType(type) {
      CHECK(contains({IMP, GUARD_POST, CUT_TREE}, type));
    }

  };
  static vector<Collective::BuildInfo> initialBuildInfo;
  static vector<Collective::BuildInfo> normalBuildInfo;

  vector<Collective::BuildInfo>& getBuildInfo() const;

  const static map<ResourceId, ResourceInfo> resourceInfo;

  ViewObject getResourceViewObject(ResourceId id) const;

  map<ResourceId, int> credit;

  bool isDownstairsVisible() const;
  bool isThroneBuilt() const;
  struct CostInfo {
    ResourceId id;
    int value;
  };
  void markSquare(Vec2 pos, SquareType type, CostInfo);
  void unmarkSquare(Vec2 pos);
  void removeTask(Task*);
  void addTask(PTask, Creature*);
  void addTask(PTask);
  int numGold(ResourceId) const;
  void takeGold(CostInfo);
  void returnGold(CostInfo);
  CostInfo getImpCost() const;
  bool canBuildDoor(Vec2 pos) const;
  bool canPlacePost(Vec2 pos) const;
  void freeFromGuardPost(const Creature*);
  vector<pair<Item*, Vec2>> getTrapItems(TrapType, set<Vec2> = {}) const;
  void updateTraps();
  ItemPredicate unMarkedItems(ItemType) const;
  MarkovChain<MinionTask> getTasksForMinion(Creature* c);
  CreatureFactory minionFactory;
  CreatureFactory undeadFactory;
  vector<Creature*> creatures;
  vector<Creature*> minions;
  vector<Creature*> imps;
  vector<Creature*> vampires;
  vector<PTask> tasks;
  set<const Item*> markedItems;
  map<Vec2, Task*> marked;
  map<Task*, Creature*> taken;
  map<Creature*, Task*> taskMap;
  map<Task*, CostInfo> completionCost;
  struct TrapInfo {
    TrapType type;
    bool armed;
    bool marked;
  };
  map<Vec2, TrapInfo> traps;
  map<TrapType, vector<Vec2>> trapMap;
  map<Creature*, MarkovChain<MinionTask>> minionTasks;
  map<const Creature*, string> minionTaskStrings;
  set<pair<Creature*, Task*>> locked;
  map<SquareType, set<Vec2>> mySquares;
  set<Vec2> myTiles;
  Level* level;
  Creature* heart = nullptr;
  mutable map<const Level*, MapMemory> memory;
  int currentButton = 0;
  bool gatheringTeam = false;
  vector<Creature*> team;
  map<const Level*, Vec2> teamLevelChanges;
  map<const Level*, Vec2> levelChangeHistory;
  Creature* possessed = nullptr;
  MinionEquipment minionEquipment;
  struct GuardPostInfo {
    const Creature* attender;
  };
  map<Vec2, GuardPostInfo> guardPosts;
  Optional<Vec2> throneMarked;
};

#endif
