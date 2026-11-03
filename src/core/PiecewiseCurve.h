#pragma once

#include <cstdint>

struct PiecewiseCurve
{
  double x0 =  32;
  uint8_t y0 = 100;
  double x1 =  96;
  uint8_t y1 =   0;
};

float evaluateCurveWeight(const PiecewiseCurve& c, double x);