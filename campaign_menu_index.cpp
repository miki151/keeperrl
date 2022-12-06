#include "campaign_menu_index.h"

using namespace CampaignMenuElems;

void CampaignMenuIndex::left() {
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
      [&](Biome b) {
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

void CampaignMenuIndex::right(int numBiomes) {
  visit(
      [&](None) {
        assign(Help{});
      },
      [&](Help) {
        assign(Biome{});
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
      [&](Biome) {
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
      [&](Biome) {
        assign(Confirm{});
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

