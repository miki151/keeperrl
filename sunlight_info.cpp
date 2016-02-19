#include "stdafx.h"
#include "sunlight_info.h"


const char* SunlightInfo::getText() {
  switch (state) {
    case SunlightState::NIGHT: return "night";
    case SunlightState::DAY: return "day";
  }
}


