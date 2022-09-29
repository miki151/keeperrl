#include "util.h"
#include "view.h"

constexpr int avatarsPerPage = 10;

namespace AvatarIndexElems {
  struct RoleIndex {
    int value;
    COMPARE_ALL(value)
  };
  struct GenderIndex {
    int value;
    COMPARE_ALL(value)
  };
  struct NameIndex {
    int value;
    COMPARE_ALL(value)
  };
  struct CreatureIndex {
    Vec2 value;
    COMPARE_ALL(value)
  };
  EMPTY_STRUCT(EnterNameIndex);
  EMPTY_STRUCT(RollNameIndex);
  EMPTY_STRUCT(StartNewGameIndex);
  struct BottomButtonsIndex {
    int value;
    COMPARE_ALL(value)
  };
  struct PageButtonsIndex {
    int value;
    COMPARE_ALL(value)
  };
  using AvatarIndexVariant = variant<RoleIndex, GenderIndex, NameIndex, CreatureIndex, EnterNameIndex, RollNameIndex,
      StartNewGameIndex, BottomButtonsIndex, PageButtonsIndex>;
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