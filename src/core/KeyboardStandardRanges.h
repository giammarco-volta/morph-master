#pragma once

enum class KeyboardRangeId
{
  Full,
  K88,
  K76,
  K73,
  K61,
  K49,
  K37,
  K25
};

struct KeyboardRange
{
  KeyboardRangeId id;
  const char* name;
  int minNote;
  int maxNote;
};

static constexpr KeyboardRange kKeyboardRanges[] =
{
  { KeyboardRangeId::Full, "Full (0-127)",      0, 127 },
  { KeyboardRangeId::K88,  "88 keys (21-108)", 21, 108 },
  { KeyboardRangeId::K76,  "76 keys (28-103)", 28, 103 },
  { KeyboardRangeId::K73,  "73 keys (24-96)",  24,  96 },
  { KeyboardRangeId::K61,  "61 keys (36-96)",  36,  96 },
  { KeyboardRangeId::K49,  "49 keys (36-84)",  36,  84 },
  { KeyboardRangeId::K37,  "37 keys (48-84)",  48,  84 },
  { KeyboardRangeId::K25,  "25 keys (48-72)",  48,  72 }
};