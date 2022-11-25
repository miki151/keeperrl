#include "stdafx.h"
#include "tutorial_state.h"

string rightClick(bool controller) {
  return controller ? "left trigger" : "right click";
}

string getMessage(TutorialState state, bool controller) {
  switch (state) {
    case TutorialState::WELCOME:
      return "Welcome to the KeeperRL tutorial!\n \n"
          "Together we will work to build a small dungeon, and assemble a military force. Nearby us lies "
          "a small village inhabited mostly by innocents, which we shall conquer.\n \n"
          "This should get you up to speed with the game!";
    case TutorialState::INTRO:
      return "Let's check out some things that you see on the map. The little wizard wearing a red robe is you, "
          "the Keeper.\n \n"
          "Remember that KeeperRL features perma-death, which means that you can't reload the game after a failure. "
          "If your Keeper dies, then the game is over, so be careful!";
    case TutorialState::INTRO2:
      return "The four little creatures are your imps. They are here to perform your orders. Try hovering the mouse "
          "over other things on the map, and notice the hints in the lower right corner.";
    case TutorialState::CUT_TREES:
      return "Great things come from small beginnings. Your first task is to gather some wood! "_s +
          (controller ? "Open the buildings menu using the left trackpad. " : "") +
          "Select the \"Dig or cut tree\" order and click on "
          "a few trees to order your imps to cut them down.\n \nWhen you're done, " +
          (controller ? "pull the left trigger or press B to clear the order."
              : "click the right mouse button or press escape to clear the order.");
    case TutorialState::BUILD_STORAGE:
      return "You need to store the wood somewhere before you can use it. Select resource storage and designate it "
          "by clicking on the map. If you want to remove it, just click again.\n \n"
          "Create at least a 3x3 storage area.";
    case TutorialState::CONTROLS1:
      return "Time to learn a few controls! Try scrolling the map using "_s +
          (controller ? "the right joystick.\n \n" : "the arrow keys or by right clicking on the map "
          "and dragging it.\n \n") +
          "Press "_s + (controller ? "A" : "SPACE") + " to pause and continue the game. "
          "You can still give orders and use all controls while the game is paused.";
    case TutorialState::CONTROLS2:
      return "Try zooming in and out of the map "_s + (controller ? "pressing the left joystick"
          : "using the 'z' key or mouse wheel.");
    case TutorialState::GET_200_WOOD:
      return "Cut some more trees until you have gathered at least 200 wood.";
    case TutorialState::DIG_ROOM:
      return "Time to strike the earth! Dig out a tunnel and a room, as shown by the yellow highlight.";
    case TutorialState::BUILD_DOOR:
      return "Build a door at the entrance to your dungeon. This will slow down potential intruders, "
          "as they will need to destroy it before they can enter. "
          "Your minions can pass through doors freely, unless you lock a door by clicking on it.\n \n"
          "Try locking and unlocking your new door.";
    case TutorialState::BUILD_LIBRARY:
      return "The first room that you need to build is a library. This is where the Keeper and other minions "
          "will learn spells, and train their spell damage attribute. Place 6 bookcases "
          "in the new room as highlighted. Remember that bookcases and other furniture block your minions' movement.";
    case TutorialState::DIG_2_ROOMS:
      return "Dig out some more rooms. "
          "If the library is blocking your tunnels, use the 'remove construction' order to get it out of the way.\n \n"
          "Change the speed of the game using the keys "_s + (controller ? "L4 and R4" : "1-4 or by clicking in the "
          "lower left corner") + " if it's taking too long.";
    case TutorialState::ACCEPT_IMMIGRANT:
      return "Your dungeon has attracted a goblin warrior. "
          "Before your new minion joins you, you must fulfill a few requirements. "
          "Hover the mouse over the immigrant icon to see them. "
          "Once you are ready, accept the immigrant with a click. Click on the '?' icon immediately to the left "
          "to learn more about immigration.";
    case TutorialState::TORCHES:
      return "We need to make sure that your minions have good working conditions when studying and training. Place some "
          "torches in your rooms to light them up. Hover the mouse over the book shelves and training dummies "
          "and look in the lower right corner to check if they have enough light.";
    case TutorialState::FLOORS:
      return "Minions are also more efficient if there is a nice floor where they are working. For now you can only "
          "afford wooden floors, but they should do.\n \n"
          "Make sure you have enough wood!";
    case TutorialState::BUILD_WORKSHOP:
      return "Your minions will need equipment, such as weapons, armor, and consumables, to be more deadly in "
          "combat.\n \n"
          "Build at least 2 workshop stands in your dungeon. It's best to dig out a dedicated room for them. "
          "You will also need a storage area for equipment. Place it somewhere near your workshop.";
    case TutorialState::SCHEDULE_WORKSHOP_ITEMS:
      return "Weapons are the most important piece of equipment, because unarmed, your minions have little chance "
          "against an enemy. Open the workshop menu by clicking on any of your workshop stands and schedule the "
          "production of a club.";
    case TutorialState::ORDER_CRAFTING:
      return "To have your item produced, order your goblin to pick up crafting. "
          "Click and drag him onto a workshop stand. "
          "Pausing the game will make it a bit easier. Also, make sure you have enough wood!\n \n"
          "You can check the progress of production when you click on the workshop.";
    case TutorialState::EQUIP_WEAPON:
      return "Your minions will automatically pick up weapons and other equipment from the storage, "
          "but you can also control it manually. Click on your goblin, and on his weapon slot to assign him the "
          "club that he has just produced.\n \n"
          "This way you will order him to go and pick it up.\n \n";
    case TutorialState::ACCEPT_MORE_IMMIGRANTS:
      return "You are ready to grow your military force. Three more goblin warrior immigrants should do.\n \n"
          "You can also invite goblin artificers, which don't fight, but are excellent craftsmen.";
    case TutorialState::EQUIP_ALL_FIGHTERS:
      return "Craft clubs for all of your warriors, and have them equipped. They will be needed soon.";
    case TutorialState::CREATE_TEAM:
      return "Your tiny army is ready! Assemble a team by dragging your warriors onto the [new team] button. You can "
          "drag them straight from the map or from the minion menu.\n \n"
          "Create a team of 4 goblin warriors.";
    case TutorialState::CONTROL_TEAM:
      return "Time to take control over your team. Select the team, and one of the team members as the leader, "
          "and click [Control].";
    case TutorialState::CONTROL_MODE_MOVEMENT:
      return "You are now in control of a minion, and the game has become turn-based. Try moving around using "_s +
          (controller ? "the left joystick and the [A] button" : "the arrow keys") + " or by clicking somewhere on the map. "
          "You can scroll the map " + (controller ? "using the right joystick" : "by dragging it with the right mouse button.") +
          "\n \nNotice the rest of your team following you.";
    case TutorialState::FULL_CONTROL:
      return "You can take control over all team members in a tactical situation. To do this "_s +
          (controller ? "select \"Toggle control mode\" using the left trackpad" : "click on the [Control mode] "
          "button in the upper left corner or press [G]") + ". Clicking again will go back to controlling only the team "
          "leader.";
    case TutorialState::DISCOVER_VILLAGE:
      return "It's time to discover the whereabouts of the nearby human village. "_s +
          (controller ? "Press the right joystick to open the minimap" : " Click on the minimap in the upper "
          "right corner") + ". The approximate location of the village is marked by a '?'. Take your team there.";
    case TutorialState::KILL_VILLAGE:
      return "Your team has arrived at the village. The only right thing to do in this situation is to find and "
          "exterminate all inhabitants!\n \n"
          "Remember to break into every house by destroying the door.";
    case TutorialState::LOOT_VILLAGE:
      return "There is a nice pile of treasure in one of the houses. Pick it all up by entering the tiles containing "
          "the loot, and use the \"Lying here:\" menu in the upper left corner.";
    case TutorialState::LEAVE_CONTROL:
      return "To relinquish control of your team, "_s + (controller ? "press [Y]."
        : "click the [Exit control mode] button in the upper left corner.");
    case TutorialState::SUMMARY1:
      return "You are back in the real-time mode. Your minions will now return to base and resume their normal routine. "
          "Once they are back, they will drop all the loot for the imps to take care of.";
    case TutorialState::RESEARCH:
      return "You have increased your malevolence level. This is the main meter of your progress in the game and allows "
          "you to research new technologies.\n \n"_s +
          (controller ? "Open the technology tab using the left trackpad" : "Click on the technology tab") +
          " and research something.";
    case TutorialState::HELP_TAB:
      return "KeeperRL features how-to pages for some more advanced features. "_s +
          (controller ? "Open the help tab using the left trackpad" : "Click on the help tab icon") +
          " to see them. They're likely to be useful in your future playthroughs.";
    case TutorialState::MINIMAP_BUTTONS:
      return "As the last objective, familiarize yourself with the two buttons under the minimap in the top-right corner. "
          "The first one opens the world map window, which you can use to travel to other sites when in control mode.\n \n"
          "The second one centers the map on your Keeper.";
    case TutorialState::SUMMARY2:
      return "Thank you for completing the tutorial! We hope that we have made it a bit easier for you to get into "
          "KeeperRL. We would love to hear your comments, so please drop by on the forums on Steam or at keeperrl.com "
          "some time!";
    case TutorialState::FINISHED:
      return "You should go and start a new game now, as this one exists only for the purpose of the tutorial.\n \n"
          "Press "_s + (controller ? "the menu button" : "Escape") + " and abandon this game. ";
  }
}
