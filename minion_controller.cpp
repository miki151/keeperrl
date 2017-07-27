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

class MinionController : public Player {
  public:
  MinionController(WCreature c, SMapMemory memory, WPlayerControl ctrl, SMessageBuffer messages,
                   SVisibilityMap visibilityMap, STutorial tutorial)
      : Player(c, false, memory, messages, visibilityMap, tutorial), control(ctrl) {}

  virtual vector<CommandInfo> getCommands() const override {
    auto tutorial = control->getTutorial();
    return concat(Player::getCommands(), {
      {PlayerInfo::CommandInfo{"Leave creature", 'u', "Leave creature and order team back to base.", true,
            tutorial && tutorial->getHighlights(getGame()).contains(TutorialHighlight::LEAVE_CONTROL)},
       [] (Player* player) { dynamic_cast<MinionController*>(player)->unpossess(); }, true},
      {PlayerInfo::CommandInfo{"Toggle control all", none, "Control all team members.", true},
       [] (Player* player) { dynamic_cast<MinionController*>(player)->control->toggleControlAllTeamMembers(); }, getTeam().size() > 1},
      {PlayerInfo::CommandInfo{"Switch control", 's', "Switch control to a different team member.", true},
       [] (Player* player) { dynamic_cast<MinionController*>(player)->swapTeam(); }, getTeam().size() > 1},
      {PlayerInfo::CommandInfo{"Absorb", 'a',
          "Absorb a friendly creature and inherit its attributes. Requires the absorbtion skill.",
          getCreature()->getAttributes().getSkills().hasDiscrete(SkillId::CONSUMPTION)},
       [] (Player* player) { dynamic_cast<MinionController*>(player)->consumeAction();}, false},
    });
  }

  void consumeAction() {
    vector<WCreature> targets = control->getCollective()->getConsumptionTargets(getCreature());
    vector<WCreature> actions;
    for (auto target : targets)
      if (auto action = getCreature()->consume(target))
        actions.push_back(target);
    if (actions.size() == 1 && getView()->yesOrNoPrompt("Really absorb " + actions[0]->getName().the() + "?")) {
      tryToPerform(getCreature()->consume(actions[0]));
    } else
    if (actions.size() > 1) {
      auto dir = chooseDirection("Which direction?");
      if (!dir)
        return;
      if (WCreature c = getCreature()->getPosition().plus(*dir).getCreature())
        if (targets.contains(c) && getView()->yesOrNoPrompt("Really absorb " + c->getName().the() + "?"))
          tryToPerform(getCreature()->consume(c));
    }
  }

  void unpossess() {
    control->leaveControl();
  }

  bool swapTeam() {
    return control->swapTeam();
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

  virtual void onFellAsleep() override {
    getGame()->getView()->presentText("Important!", "You fall asleep. You loose control of your minion.");
    unpossess();
  }

  virtual vector<WCreature> getTeam() const override {
    return control->getTeam(getCreature());
  }

  virtual MessageGenerator& getMessageGenerator() const override {
    static MessageGenerator messageGeneratorSecond(MessageGenerator::SECOND_PERSON);
    static MessageGenerator messageGeneratorThird(MessageGenerator::THIRD_PERSON);

    if (control->getControlled().size() == 1)
      return messageGeneratorSecond;
    else
      return messageGeneratorThird;
  }

  SERIALIZE_ALL(SUBCLASS(Player), control)
  SERIALIZATION_CONSTRUCTOR(MinionController)

  private:
  WPlayerControl SERIAL(control);
};

REGISTER_TYPE(MinionController)


PController getMinionController(WCreature c, SMapMemory m, WPlayerControl ctrl, SMessageBuffer buf, SVisibilityMap v,
    STutorial t) {
  return makeOwner<MinionController>(c, m, ctrl, buf, v, t);
}
