#pragma once

#include "util.h"

namespace CampaignMenuElems {
  EMPTY_STRUCT(RetiredDungeons);
  EMPTY_STRUCT(Settings);
  EMPTY_STRUCT(ChangeMode);
  EMPTY_STRUCT(Help);
  EMPTY_STRUCT(Confirm);
  EMPTY_STRUCT(RollMap);
  EMPTY_STRUCT(Back);
  EMPTY_STRUCT(None);
  using CampaignMenuIndexVariant = variant<RetiredDungeons, Settings, ChangeMode, Help, Confirm, RollMap, Back, None>;
}

struct CampaignMenuIndex : public CampaignMenuElems::CampaignMenuIndexVariant {
  using CampaignMenuElems::CampaignMenuIndexVariant::CampaignMenuIndexVariant;
  template <typename T>
  void assign(T elem) {
    *((CampaignMenuElems::CampaignMenuIndexVariant*)this) = std::move(elem);
  }
  void left();
  void right();
  void up();
  void down();
};