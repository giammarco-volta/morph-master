#pragma once

#include "TrackGroupId.h"
#include "GroupPlacement.h"
#include <QString>

inline QString groupName(TrackGroupId g)
{
  switch (g)
  {
  case TrackGroupId::None:        return "--";
  case TrackGroupId::Forte:       return "Forte";
  case TrackGroupId::TrebleForte: return "Treble-Forte";
  case TrackGroupId::Treble:      return "Treble";
  case TrackGroupId::TreblePiano: return "Treble-Piano";
  case TrackGroupId::Piano:       return "Piano";
  case TrackGroupId::BassPiano:   return "Bass-Piano";
  case TrackGroupId::Bass:        return "Bass";
  case TrackGroupId::BassForte:   return "Bass-Forte";
  default:                        return "?";
  }
}

inline GroupPlacement groupPlacement(TrackGroupId g)
{
  switch (g)
  {
  case TrackGroupId::Forte:   return GroupPlacement::VelocityOnly;
  case TrackGroupId::Piano:   return GroupPlacement::VelocityOnly;

  case TrackGroupId::Treble:  return GroupPlacement::KeyOnly;
  case TrackGroupId::Bass:    return GroupPlacement::KeyOnly;

  case TrackGroupId::TrebleForte:
  case TrackGroupId::TreblePiano:
  case TrackGroupId::BassPiano:
  case TrackGroupId::BassForte:
  default:
    return GroupPlacement::BothAxes;
  }
}