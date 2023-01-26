#pragma once

#include "util.h"
#include "view.h"

constexpr int avatarsPerPage = 10;

namespace AvatarIndexElems {
  struct RoleIndex {
    int SERIAL(value);
    COMPARE_ALL(value)
  };
  struct GenderIndex {
    int SERIAL(value);
    COMPARE_ALL(value)
  };
  struct NameIndex {
    int SERIAL(value);
    COMPARE_ALL(value)
  };
  struct CreatureIndex {
    int SERIAL(value);
    COMPARE_ALL(value)
  };
  EMPTY_STRUCT(EnterNameIndex);
  EMPTY_STRUCT(RollNameIndex);
  EMPTY_STRUCT(StartNewGameIndex);
  struct BottomButtonsIndex {
    int SERIAL(value);
    COMPARE_ALL(value)
  };
  struct PageButtonsIndex {
    int SERIAL(value);
    COMPARE_ALL(value)
  };
  EMPTY_STRUCT(None);
  using AvatarIndexVariant = variant<RoleIndex, GenderIndex, NameIndex, CreatureIndex, EnterNameIndex, RollNameIndex,
      StartNewGameIndex, BottomButtonsIndex, PageButtonsIndex, None>;
}

struct AvatarIndex : public AvatarIndexElems::AvatarIndexVariant {
  template <typename T>
  void assign(T elem) {
    *((AvatarIndexElems::AvatarIndexVariant*)this) = std::move(elem);
  }
  void left(const vector<View::AvatarData>& avatars, int chosenAvatar, View::AvatarRole chosenRole,
      int avatarPage);
  void right(const vector<View::AvatarData>& avatars, int chosenAvatar, View::AvatarRole chosenRole,
      int avatarPage);
  void up(const vector<View::AvatarData>& avatars, int chosenAvatar, View::AvatarRole chosenRole,
      int avatarPage);
  void down(const vector<View::AvatarData>& avatars, int chosenAvatar, View::AvatarRole chosenRole,
      int avatarPage);
};