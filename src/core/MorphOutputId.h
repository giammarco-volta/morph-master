#pragma once

#include <cstdint>

enum class MorphOutputId : uint8_t
{

  Loud = 0,
  HighLoud,
  High,
  HighSoft,
  Soft,
  LowSoft,
  Low,
  LowLoud,
  None,

  Count
};

struct MorphOutputProfile
{
  static MorphOutputProfile GetProfile(MorphOutputId g)
  {
    MorphOutputProfile p;

    if (g == MorphOutputId::None)
      return p;

    p.useKey =
         g != MorphOutputId::Loud
      && g != MorphOutputId::Soft;

    p.useVelocity =
         g != MorphOutputId::High
      && g != MorphOutputId::Low;

    p.invertKey =
         g == MorphOutputId::HighSoft
      || g == MorphOutputId::High
      || g == MorphOutputId::HighLoud;

    p.invertVelocity =
         g == MorphOutputId::HighLoud
      || g == MorphOutputId::Loud
      || g == MorphOutputId::LowLoud;

    return p;
  }
  bool useKey = false;
  bool invertKey = false;

  bool useVelocity = false;
  bool invertVelocity = false;
};