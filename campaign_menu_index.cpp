#include "campaign_menu_index.h"

using namespace CampaignMenuElems;

void CampaignMenuIndex::left(const View::CampaignOptions&) {
  visit(
      [&](Help) {
        assign(RetiredDungeons{});
      },
      [&](ChangeMode) {
        assign(Help{});
      },
      [&](Biome b) {
        if (b.value > 0) {
          --b.value;
          assign(b);
        } else
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

void CampaignMenuIndex::right(const View::CampaignOptions& o) {
  visit(
      [&](Help) {
        assign(Biome{0});
      },
      [&](RetiredDungeons) {
        assign(Help{});
      },
      [&](Settings) {
        assign(Help{});
      },
      [&](Biome b) {
        if (b.value < o.biomes.size() - 1) {
          ++b.value;
          assign(b);
        }
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

void CampaignMenuIndex::up(const View::CampaignOptions&) {
  visit(
      [&](Help) {
        assign(ChangeMode{});
      },
      [&](Settings) {
        assign(RetiredDungeons{});
      },
      [&](RetiredDungeons) {
        assign(ChangeMode{});
      },
      [&](Biome b) {
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

void CampaignMenuIndex::down(const View::CampaignOptions&) {
  visit(
      [&](Help) {
        assign(Confirm{});
      },
      [&](ChangeMode) {
        assign(Help{});
      },
      [&](Biome b) {
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

