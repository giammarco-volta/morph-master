#pragma once
#include "PiecewiseCurve.h"
#include <array>
#include <algorithm>
#include <cmath>

struct CurveCcAssignment {
  int cc = -1;
  double minimum = 0;
  double maximum = 127;
  int channel = -1; // -1 follows the note input channel; otherwise 0..15.
};
using CurveCcAssignments = std::array<CurveCcAssignment, 4>;
inline CurveCcAssignments defaultCurveCcAssignments() {
  return {{{-1,0,127}, {-1,1,127}, {-1,0,127}, {-1,1,127}}};
}
inline bool isAssignableCurveCc(int cc) {
  if (cc < 0 || cc >= 120) return false;
  switch (cc) {
    case 0: case 6: case 7: case 10: case 11: case 32: case 38:
    case 64: case 71: case 74: case 91: case 93:
    case 96: case 97: case 98: case 99: case 100: case 101: return false;
    default: return true;
  }
}
inline int curveCcChannel(const CurveCcAssignment& assignment, int noteChannel) {
  return assignment.channel<0 ? noteChannel : assignment.channel;
}
inline bool isAssignableCurveCc(int cc, int channel, int noteChannel) {
  return cc>=0 && cc<=127 && (channel!=noteChannel || isAssignableCurveCc(cc));
}
inline void sanitizeCurveCcAssignments(CurveCcAssignments& assignments, int noteChannel=0) {
  std::array<std::array<bool,128>,16> used{};
  for (int i=0; i<4; ++i) {
    auto& a=assignments[i];
    a.channel=std::clamp(a.channel,-1,15);
    const int channel=curveCcChannel(a,noteChannel);
    if (!isAssignableCurveCc(a.cc,channel,noteChannel) || used[channel][a.cc]) a.cc=-1;
    else used[channel][a.cc]=true;
    const double lower = i%2 ? 1.0 : 0.0;
    a.minimum=std::clamp(std::isfinite(a.minimum) ? a.minimum : lower, lower, 127.0);
    a.maximum=std::clamp(std::isfinite(a.maximum) ? a.maximum : 127.0, a.minimum, 127.0);
  }
}

// Position is the segment center; slope 0..127 maps width 127..1.
inline double curveParameter(const PiecewiseCurve& curve, bool slope) {
  return slope ? (127.0-(curve.x1-curve.x0))*127.0/126.0
               : (curve.x0+curve.x1)*0.5;
}
inline void setCurveParameter(PiecewiseCurve& curve, bool slope, double value,
                              double low=0, double high=127) {
  double width=std::clamp(curve.x1-curve.x0,1.0,high-low);
  double center=std::clamp((curve.x0+curve.x1)*0.5,low+width/2,high-width/2);
  if(slope) width=std::clamp(127.0-std::clamp(value,0.0,127.0)*126.0/127.0,
                            1.0,2.0*std::min(center-low,high-center));
  else center=std::clamp(value,low+width/2,high-width/2);
  curve.x0=center-width/2;
  curve.x1=center+width/2;
}
// Compatibility for monitor bindings: the stored curve is now the only curve.
struct CurveTransform {
  double scale=1, offset=0;
  double inputFor(double x) const { return x; }
  double outputFor(double x) const { return x; }
};
