#include "campaign_menu_index.h"

void CampaignMenuIndex::left() {
  using namespace CampaignMenuElems;
  visit(
      [&](None) {
        assign(Help{});
      },
      [&](Help) {
        assign(RetiredDungeons{});
      },
      [&](ChangeMode) {
        assign(Help{});
      },
      [&](RollMap b) {
        assign(Confirm{});
      },
      [&](Back b) {
        assign(RollMap{});
      },
      [](const auto&) {}
  );
}

void CampaignMenuIndex::right() {
  using namespace CampaignMenuElems;
  visit(
      [&](None) {
        assign(Help{});
      },
      [&](RetiredDungeons) {
        assign(Help{});
      },
      [&](Settings) {
        assign(Help{});
      },
      [&](Confirm b) {
        assign(RollMap{});
      },
      [&](RollMap b) {
        assign(Back{});
      },
      [](const auto&) {}
  );
}

void CampaignMenuIndex::up() {
  using namespace CampaignMenuElems;
  visit(
      [&](None) {
        assign(Help{});
      },
      [&](Help) {
        assign(ChangeMode{});
      },
      [&](Settings) {
        assign(RetiredDungeons{});
      },
      [&](RetiredDungeons) {
        assign(ChangeMode{});
      },
      [&](Confirm b) {
        assign(Help{});
      },
      [&](RollMap b) {
        assign(Help{});
      },
      [&](Back b) {
        assign(Help{});
      },
      [](const auto&) {}
  );
}

void CampaignMenuIndex::down() {
  using namespace CampaignMenuElems;
  visit(
      [&](None) {
        assign(Help{});
      },
      [&](Help) {
        assign(Confirm{});
      },
      [&](ChangeMode) {
        assign(Help{});
      },
      [&](RetiredDungeons) {
        assign(Settings{});
      },
      [&](Settings) {
        assign(Confirm{});
      },
      [](const auto&) {}
  );
}

