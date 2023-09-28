#include "avatar_index.h"

static int getNumAvatars(const vector<View::AvatarData>& avatars, int avatarPage) {
  int numAvatars = avatars.size();
  return min(10, numAvatars - avatarPage * avatarsPerPage);
}

static int getNumPages(const vector<View::AvatarData>& avatars) {
  int numAvatars = avatars.size();
  return (numAvatars + avatarsPerPage - 1) / avatarsPerPage;
}

void AvatarIndex::left(const vector<View::AvatarData>& avatars, int chosenAvatar, int avatarPage) {
  using namespace AvatarIndexElems;
  int numAvatars = getNumAvatars(avatars, avatarPage);
  visit(
    [&](None) {
      assign(RoleIndex{0});
    },
    [&](const RoleIndex& i) {
      if (i.value > 0)
        assign(RoleIndex{i.value - 1});
    },
    [&](const GenderIndex& i) {
      switch (avatars[chosenAvatar].genderNames.size()) {
        case 2: assign(GenderIndex{(i.value + 1) % 2}); break;
        case 1: assign(GenderIndex{0}); break;
        case 0: assign(RoleIndex{0}); break;
      }
    },
    [&](CreatureIndex i) {
      if (i.value == Vec2(0, 0))
        assign(RoleIndex{1});
      else if (i.value == Vec2(0, 1) && !!avatars[chosenAvatar].settlementNames)
        assign(RollNameIndex{});
      else if (i.value == Vec2(0, 1))
        assign(GenderIndex{avatars[chosenAvatar].genderNames.size() - 1});
      else {
        --i.value.x;
        assign(i);
      }
    },
    [&](RollNameIndex) {
      assign(EnterNameIndex{});
    },
    [&](const BottomButtonsIndex& i) {
      assign(BottomButtonsIndex{(i.value + 2) % 3});
    },
    [&](PageButtonsIndex i) {
      if (i.value > 0 && avatarPage > 0) {
        --i.value;
        assign(i);
      } else {
        if (numAvatars > avatarsPerPage / 2)
          assign(CreatureIndex{Vec2(numAvatars - avatarsPerPage / 2 - 1, 1)});
        else
          assign(CreatureIndex{Vec2(numAvatars - 1, 0)});
      }
    },
    [](const auto&) {}
  );
}

void AvatarIndex::right(const vector<View::AvatarData>& avatars, int chosenAvatar, int avatarPage) {
  using namespace AvatarIndexElems;
  int numAvatars = getNumAvatars(avatars, avatarPage);
  auto genderRight = [&] {
    if (numAvatars > avatarsPerPage / 2)
      assign(CreatureIndex{Vec2(0, 1)});
    else
      assign(CreatureIndex{Vec2(0, 0)});
  };
  visit(
    [&](None) {
      assign(RoleIndex{0});
    },
    [&](const RoleIndex& i) {
      if (i.value == 0)
        assign(RoleIndex{1});
      else
        assign(CreatureIndex{Vec2(0, 0)});
    },
    [&](const GenderIndex& i) {
      if (i.value < avatars[chosenAvatar].genderNames.size() - 1)
        assign(GenderIndex{i.value + 1});
      else
        genderRight();
    },
    [&](RollNameIndex) {
      genderRight();
    },
    [&](EnterNameIndex) {
      assign(RollNameIndex{});
    },
    [&](CreatureIndex i) {
      if ((i.value.y == 0 && i.value.x < min(numAvatars, avatarsPerPage / 2) - 1) ||
          (i.value.y == 1 && i.value.x < numAvatars - avatarsPerPage / 2 - 1)) {
        ++i.value.x;
        assign(i);
      } else if (avatarPage > 0)
        assign(PageButtonsIndex{0});
      else if (getNumPages(avatars) > 0)
        assign(PageButtonsIndex{1});
    },
    [&](PageButtonsIndex i) {
      if (i.value == 0 && avatarPage < getNumPages(avatars) - 1) {
        ++i.value;
        assign(i);
      }
    },
    [&](const BottomButtonsIndex& i) {
      assign(BottomButtonsIndex{(i.value + 1) % 3});
    },
    [](const auto&) {}
  );
}

void AvatarIndex::up(const vector<View::AvatarData>& avatars, int chosenAvatar, int avatarPage) {
  using namespace AvatarIndexElems;
  int numAvatars = getNumAvatars(avatars, avatarPage);
  auto nameUp = [&] (int row) {
    if (!avatars[chosenAvatar].settlementNames)
      assign(GenderIndex{min(row, avatars[chosenAvatar].genderNames.size() - 1)});
    else
      assign(RoleIndex{row});
  };
  visit(
    [&](None) {
      assign(RoleIndex{0});
    },
    [&](const GenderIndex& i) {
      assign(RoleIndex{i.value});
    },
    [&](RollNameIndex) {
      nameUp(1);
    },
    [&](EnterNameIndex) {
      nameUp(0);
    },
    [&](StartNewGameIndex) {
      assign(RollNameIndex{});
    },
    [&](CreatureIndex i) {
      if (i.value.y == 1) {
        --i.value.y;
        assign(i);
      }
    },
    [&](const BottomButtonsIndex&) {
      assign(StartNewGameIndex{});
    },
    [](const auto&) {}
  );
}

void AvatarIndex::down(const vector<View::AvatarData>& avatars, int chosenAvatar, int avatarPage) {
  using namespace AvatarIndexElems;
  int numAvatars = getNumAvatars(avatars, avatarPage);
  visit(
    [&](None) {
      assign(RoleIndex{0});
    },
    [&](const RoleIndex& i) {
      if (!avatars[chosenAvatar].settlementNames)
        assign(GenderIndex{min(avatars[chosenAvatar].genderNames.size() - 1, i.value)});
      else
        assign(i.value == 0 ? AvatarIndexVariant{EnterNameIndex{}} : AvatarIndexVariant{RollNameIndex{}});
    },
    [&](const GenderIndex& i) {
      assign(i.value == 0 ? AvatarIndexVariant{EnterNameIndex{}} : AvatarIndexVariant{RollNameIndex{}});
    },
    [&](RollNameIndex) {
      assign(StartNewGameIndex{});
    },
    [&](EnterNameIndex) {
      assign(StartNewGameIndex{});
    },
    [&](CreatureIndex i) {
      if (i.value.y == 0 && numAvatars > avatarsPerPage / 2) {
        ++i.value.y;
        i.value.x = min(i.value.x, numAvatars - avatarsPerPage / 2 - 1);
        assign(i);
      } else
        assign(StartNewGameIndex{});
    },
    [&](const StartNewGameIndex&) {
      assign(BottomButtonsIndex{0});
    },
    [](const auto&) {}
  );
}
