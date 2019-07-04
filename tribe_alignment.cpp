#include "stdafx.h"
#include "tribe_alignment.h"

const char* getName(TribeAlignment alignment) {
  switch (alignment) {
    case TribeAlignment::EVIL:
      return "evil";
    case TribeAlignment::LAWFUL:
      return "less evil";
  }
}
