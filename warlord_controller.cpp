#include "enums.h"
#include "event_listener.h"
#include "stdafx.h"
#include "warlord_controller.h"
#include "player.h"
#include "creature.h"
#include "time_queue.h"
#include "message_generator.h"
#include "game.h"
#include "map_memory.h"
#include "view_index.h"
#include "message_buffer.h"
#include "visibility_map.h"
#include "unknown_locations.h"
#include "view.h"
#include "game_event.h"
#include "creature_name.h"
#include "collective.h"
#include "collective_name.h"

class WarlordController : public Player, public EventListener<WarlordController> {
  public:
  using TeamOrders = EnumSet<TeamOrder>;
  using Team = vector<Creature*>;
  WarlordController(shared_ptr<vector<Creature*>> team, shared_ptr<TeamOrders> teamOrders)
      : WarlordController((*team)[0], team, *team, make_shared<MapMemory>(),
        make_shared<MessageBuffer>(), make_shared<VisibilityMap>(), make_shared<UnknownLocations>(),
        teamOrders) {
  }

  WarlordController(Creature* c, shared_ptr<Team> team, Team originalTeam, shared_ptr<MapMemory> memory,
        shared_ptr<MessageBuffer> messages, shared_ptr<VisibilityMap> visibility, shared_ptr<UnknownLocations> locations,
        shared_ptr<TeamOrders> orders)
      : Player(c, memory, messages, visibility, locations), team(team), originalTeam(originalTeam), teamOrders(orders) {
  }

  void onEvent(const GameEvent& event) {
    using namespace EventInfo;
    event.visit<void>(
        [&](const CreatureKilled& info) {
          if (!info.victim->isPlayer()) // only use this event to remove non-controlled team members
            team->removeElementMaybe(info.victim);
        },
        [&](const ConqueredEnemy& info) {
        },
        [&](const WonGame&) {

        },
        [](auto&) {}
    );
  }

  virtual void makeMove() override {
    if (!EventListener<WarlordController>::isSubscribed())
      EventListener<WarlordController>::subscribeTo(creature->getPosition().getModel());
    Player::makeMove();
  }

  virtual vector<TeamMemberAction> getTeamMemberActions(const Creature* member) const override {
    vector<TeamMemberAction> ret;
    ret.push_back(TeamMemberAction::CHANGE_LEADER);
    if (member->isPlayer() && member != creature && getModel()->getTimeQueue().willMoveThisTurn(member))
      ret.push_back(TeamMemberAction::MOVE_NOW);
    return ret;
  }

  virtual void onKilled(Creature* attacker) override {
    Player::onKilled(attacker);
    team->removeElement(creature);
    if (!isFullControl()) {
      vector<PlayerInfo> teamInfos;
      for (auto c : *team)
        teamInfos.push_back(PlayerInfo(c, getGame()->getContentFactory()));
      if (teamInfos.empty()) {
        getGame()->gameOver(creature, creature->getKills().size(), creature->getPoints());
        return;
      }
      optional<Creature::Id> newLeader;
      if (teamInfos.size() == 1)
        newLeader = teamInfos[0].creatureId;
      else while (!newLeader) // none is returned if player pressed ESC, so just ask repeatedly
        newLeader = getView()->chooseCreature(TStringId("CHOOSE_NEW_TEAM_LEADER"), teamInfos, TString());
      for (auto c : *team)
        if (c->getUniqueId() == *newLeader) {
          setLeader(c);
          break;
        }
    }
  }

  virtual void refreshGameInfo(GameInfo& gameInfo) const override {
    Player::refreshGameInfo(gameInfo);
    auto& info = *gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>();
    info.teamOrders = *teamOrders;
    info.controlMode = isFullControl() ? PlayerInfo::ControlMode::FULL : PlayerInfo::ControlMode::LEADER;
  }

  void control(Creature* member) {
    member->pushController(makeOwner<WarlordController>(member, team, originalTeam, levelMemory, messageBuffer, visibilityMap,
        unknownLocations, teamOrders));
  }

  void setLeader(Creature* c) {
    if (c != creature) {
      swap((*team)[0], (*team)[*team->findElement(c)]);
      if (!isFullControl()) {
        control(c);
        creature->popController();
      }
    }
  }

  void teamMemberAction(TeamMemberAction action, Creature* c) {
    switch (action) {
      case TeamMemberAction::MOVE_NOW:
        c->getPosition().getModel()->getTimeQueue().moveNow(c);
        break;
      case TeamMemberAction::CHANGE_LEADER:
        setLeader(c);
        break;
      case TeamMemberAction::REMOVE_MEMBER:
        FATAL;
        break;
    }
  }

  virtual vector<OtherCreatureCommand> getOtherCreatureCommands(Creature* c) const override {
    vector<OtherCreatureCommand> ret = Player::getOtherCreatureCommands(c);
    if (getTeam().contains(c)) {
      for (auto& action : getTeamMemberActions(c))
        ret.push_back({1, getText(action), false, [action, c](Player* player){
            (dynamic_cast<WarlordController*>(player))->teamMemberAction(action, c);}});
    }
    return ret;
  }

  virtual bool handleUserInput(UserInput input) override {
    switch (input.getId()) {
      case UserInputId::TOGGLE_CONTROL_MODE:
        if (team->size() == 1)
          return true;
        if (isFullControl()) {
          for (int i : Range(1, team->size())) {
            auto c = (*team)[i];
            if (!c->getRider() && c->isPlayer())
              c->popController();
          }
        } else
          for (int i : Range(1, team->size())) {
            auto c = (*team)[i];
            if (!c->getRider())
              control((*team)[i]);
          }
        return true;
      case UserInputId::TEAM_MEMBER_ACTION: {
        auto& info = input.get<TeamMemberActionInfo>();
        for (auto c : getTeam())
          if (c->getUniqueId() == info.memberId)
            teamMemberAction(info.action, c);
        return true;
      }
      case UserInputId::TOGGLE_TEAM_ORDER:
        teamOrders->toggle(input.get<TeamOrder>());
        return true;
      default:
        return false;
    }
  }

  bool isFullControl() const {
    return creature->getGame()->getPlayerCreatures().size() > 1;
  }

  virtual bool isTravelEnabled() const override {
    return !isFullControl();
  }

  virtual CenterType getCenterType() const override {
    if (isFullControl())
      return CenterType::STAY_ON_SCREEN;
    else
      return CenterType::FOLLOW;
  }

  virtual vector<Creature*> getTeam() const override {
    return *team;
  }

  virtual MessageGenerator& getMessageGenerator() const override {
    static MessageGenerator messageGeneratorSecond(MessageGenerator::SECOND_PERSON);
    static MessageGenerator messageGeneratorThird(MessageGenerator::THIRD_PERSON);

    if (isFullControl())
      return messageGeneratorSecond;
    else
      return messageGeneratorThird;
  }

  SERIALIZE_ALL(SUBCLASS(Player), SUBCLASS(EventListener<WarlordController>), team, originalTeam, teamOrders)
  SERIALIZATION_CONSTRUCTOR(WarlordController)

  shared_ptr<Team> SERIAL(team);
  Team SERIAL(originalTeam);
  shared_ptr<TeamOrders> SERIAL(teamOrders);
};
REGISTER_TYPE(ListenerTemplate<WarlordController>)
REGISTER_TYPE(WarlordController)
PController getWarlordController(shared_ptr<vector<Creature*>> team, shared_ptr<EnumSet<TeamOrder>> teamOrders) {
  return makeOwner<WarlordController>(std::move(team), std::move(teamOrders));
}
