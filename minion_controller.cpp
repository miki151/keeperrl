#include "stdafx.h"
#include "minion_controller.h"
#include "player.h"
#include "player_control.h"
#include "tutorial.h"
#include "creature.h"
#include "creature_attributes.h"
#include "collective.h"
#include "view.h"
#include "game.h"
#include "message_generator.h"
#include "time_queue.h"
#include "unknown_locations.h"
#include "team_order.h"
#include "collective_teams.h"
#include "view_object_action.h"
#include "collective_config.h"
#include "options.h"

class MinionController : public Player {
  public:
  MinionController(Creature* c, SMapMemory memory, WPlayerControl ctrl, SMessageBuffer messages,
                   SVisibilityMap visibilityMap, SUnknownLocations locations, STutorial tutorial)
      : Player(c, false, memory, messages, visibilityMap, locations, tutorial), control(ctrl) {}



  virtual vector<CommandInfo> getCommands() const override {
    auto tutorial = control->getTutorial();
    return concat(Player::getCommands(), {
      {PlayerInfo::CommandInfo{"Absorb", Keybinding("ABSORB"),
          "Absorb a friendly creature and inherit its attributes. Requires the absorbtion skill.",
          creature->isAffected(BuffId("CONSUMPTION_SKILL"))},
       [] (Player* player) { dynamic_cast<MinionController*>(player)->consumeAction();}, false},
    });
  }

  virtual vector<TeamMemberAction> getTeamMemberActions(const Creature* member) const override {
    vector<TeamMemberAction> ret;
    if (getGame()->getPlayerCreatures().size() == 1 && member != creature)
      ret.push_back(TeamMemberAction::CHANGE_LEADER);
    if (member->isPlayer() && member != creature
        && member->getPosition().isSameModel(creature->getPosition())
        && getModel()->getTimeQueue().willMoveThisTurn(member))
      ret.push_back(TeamMemberAction::MOVE_NOW);
    if (getTeam().size() >= 2)
      ret.push_back(TeamMemberAction::REMOVE_MEMBER);
    return ret;
  }

  virtual void refreshGameInfo(GameInfo& gameInfo) const override {
    Player::refreshGameInfo(gameInfo);
    auto& info = *gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>();
    info.canExitControlMode = true;
    if (auto team = control->getCurrentTeam()) {
      info.teamOrders.emplace();
      for (auto order : ENUM_ALL(TeamOrder))
        if (control->getTeams().hasTeamOrder(*team, creature, order))
          info.teamOrders->insert(order);
        else
          info.teamOrders->erase(order);
    } else
      info.teamOrders.reset();
  }

  virtual vector<OtherCreatureCommand> getOtherCreatureCommands(Creature* c) const override {
    vector<OtherCreatureCommand> ret = Player::getOtherCreatureCommands(c);
    if (control->isEnemy(c) && c->canBeCaptured() && control->collective->getConfig().canCapturePrisoners())
      ret.push_back({2, c->isCaptureOrdered() ?
          ViewObjectAction::CANCEL_CAPTURE_ORDER : ViewObjectAction::ORDER_CAPTURE, true,
          [c](Player*) { c->toggleCaptureOrder();}});
    if (getTeam().contains(c)) {
      for (auto& action : getTeamMemberActions(c))
        ret.push_back({1, getText(action), false, [action, id = c->getUniqueId()](Player* player){
            (dynamic_cast<MinionController*>(player))->control->teamMemberAction(action, id);}});
    }
    else if (control->collective->getCreatures().contains(c) && control->canControlInTeam(c) &&
        control->canControlInTeam(creature))
      ret.push_back({10, ViewObjectAction::ADD_TO_TEAM, false, [c](Player* player) {
          (dynamic_cast<MinionController*>(player))->control->addToCurrentTeam(c);}});
    return ret;
  }

  virtual bool handleUserInput(UserInput input) override {
    switch (input.getId()) {
      case UserInputId::TOGGLE_CONTROL_MODE:
        control->toggleControlAllTeamMembers();
        return true;
      case UserInputId::EXIT_CONTROL_MODE:
        if (creature->getPosition().getModel() == control->getModel() || canTravel()) {
          control->leaveControl();
          return true;
        } else
          return false;
      case UserInputId::TEAM_MEMBER_ACTION: {
        auto& info = input.get<TeamMemberActionInfo>();
        control->teamMemberAction(info.action, info.memberId);
        return true;
      }
      case UserInputId::TOGGLE_TEAM_ORDER: {
        if (auto team = control->getCurrentTeam()) {
          auto order = input.get<TeamOrder>();
          bool was = control->getTeams().hasTeamOrder(*team, creature, order);
          control->getTeams().setTeamOrder(*team, creature, order, !was);
        }
        return true;
      }
      default:
        return false;
    }
  }

  void consumeAction() {
    vector<Creature*> targets = control->getConsumptionTargets(creature);
    vector<Creature*> actions;
    for (auto target : targets)
      if (auto action = creature->consume(target))
        actions.push_back(target);
    if (actions.size() == 1 && getView()->yesOrNoPrompt("Really absorb " + actions[0]->getName().the() + "?")) {
      tryToPerform(creature->consume(actions[0]));
    } else
    if (actions.size() > 1) {
      auto dir = chooseDirection("Which direction?");
      if (!dir)
        return;
      if (Creature* c = creature->getPosition().plus(*dir).getCreature())
        if (targets.contains(c) && getView()->yesOrNoPrompt("Really absorb " + c->getName().the() + "?"))
          tryToPerform(creature->consume(c));
    }
  }

  virtual bool isTravelEnabled() const override {
    return control->getControlled().size() == 1;
  }

  virtual CenterType getCenterType() const override {
    if (control->getControlled().size() == 1)
      return CenterType::FOLLOW;
    else
      return CenterType::STAY_ON_SCREEN;
  }

  virtual void onLostControl() override {
    // Remove this method? Maybe it's better if the game waits for the minion to wake up instead of
    // leaving control
    //getGame()->getView()->presentText("Important!", "You lose control of your minion.");
    //control->leaveControl();
  }

  virtual vector<Creature*> getTeam() const override {
    return control->getTeam(creature);
  }

  virtual void forceSteeds() const override {
    for (auto c : getTeam())
      if (c->isAffected(LastingEffect::RIDER))
        if (auto steed = control->collective->getSteedOrRider(c))
          c->forceMount(steed);
  }

  virtual MessageGenerator& getMessageGenerator() const override {
    static MessageGenerator messageGeneratorSecond(MessageGenerator::SECOND_PERSON);
    static MessageGenerator messageGeneratorThird(MessageGenerator::THIRD_PERSON);

    if (control->getControlled().size() == 1)
      return messageGeneratorSecond;
    else
      return messageGeneratorThird;
  }

  virtual void updateUnknownLocations() override {
    control->updateUnknownLocations();
  }

  SERIALIZE_ALL(SUBCLASS(Player), control)
  SERIALIZATION_CONSTRUCTOR(MinionController)

  private:
  WPlayerControl SERIAL(control) = nullptr;
};

REGISTER_TYPE(MinionController)


PController getMinionController(Creature* c, SMapMemory m, WPlayerControl ctrl, SMessageBuffer buf, SVisibilityMap v,
    SUnknownLocations l, STutorial t) {
  return makeOwner<MinionController>(c, m, ctrl, buf, v, l, t);
}
