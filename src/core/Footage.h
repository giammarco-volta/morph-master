#pragma once

#include <cstdint>

enum class Footage : uint8_t
{
  ftg16,    //-12 semitones + 0 cents
  ftg8,     //  0 semitones + 0 cents
  ftg5_1_3, //+ 7 semitones + 2 cents
  ftg4,     //+12 semitones + 0 cents
  ftg2_2_3, //+19 semitones + 2 cents
  ftg2,     //+24 semitones + 0 cents
  ftg1_3_5, //+28 semitones -14 cents
  ftg1_1_3, //+31 semitones + 2 cents
  ftg1_1_7, //+34 semitones -31 cents
  ftg1,     //+36 semitones + 0 cents

  Count
};

static constexpr int8_t FootageTransposition[(uint8_t)Footage::Count] =
{
  -12, //ftg16
    0, //ftg8
    7, //ftg5_1_3
   12, //ftg4
   19, //ftg2_2_3
   24, //ftg2
   28, //ftg1_3_5
   31, //ftg1_1_3
   34, //ftg1_1_7
   36  //ftg1
};

static constexpr int8_t FootageDetune[(uint8_t)Footage::Count] =
{
    0, //ftg16
    0, //ftg8
    2, //ftg5_1_3
    0, //ftg4
    2, //ftg2_2_3
    0, //ftg2
  -14, //ftg1_3_5
    2, //ftg1_1_3
  -31, //ftg1_1_7
    0  //ftg1
};