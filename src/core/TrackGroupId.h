#pragma once

#include <cstdint>

enum class TrackGroupId : uint8_t
{

  Forte = 0,
  TrebleForte,
  Treble,
  TreblePiano,
  Piano,
  BassPiano,
  Bass,
  BassForte,
  None,

  Count
};

struct GroupMorphProfile
{
  static GroupMorphProfile GetProfile(TrackGroupId g)
  {
    GroupMorphProfile p;
    p.useKey =           (g != TrackGroupId::Forte && g != TrackGroupId::Piano);//All but Soft and Strong
    p.useVelocity =      (g != TrackGroupId::Treble && g != TrackGroupId::Bass);//All but High Register and Low Register
    p.invertKey =        (g == TrackGroupId::BassPiano || g == TrackGroupId::Bass || g == TrackGroupId::BassForte);//All Low Register
    p.invertVelocity =   (g == TrackGroupId::TreblePiano || g == TrackGroupId::Piano || g == TrackGroupId::BassPiano);//All Soft
    return p;
  }
  bool useKey = false;
  bool invertKey = false;

  bool useVelocity = false;
  bool invertVelocity = false;
};