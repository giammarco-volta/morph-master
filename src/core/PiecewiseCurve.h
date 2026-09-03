#pragma once

#include <cstdint>

struct PiecewiseCurve
{
  uint8_t x0 =  32;
  uint8_t y0 = 100;
  uint8_t x1 =  96;
  uint8_t y1 =   0;
};

float evaluateCurveWeight(const PiecewiseCurve& c, uint8_t x);