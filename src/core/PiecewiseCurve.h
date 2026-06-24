#pragma once

#include <cstdint>

static constexpr uint8_t numOfCurves = 4;

struct PiecewiseCurve
{
  uint8_t x0 = 32;
  uint8_t y0 = 0;
  uint8_t x1 = 96;
  uint8_t y1 = 127;
};

uint8_t evaluateCurve(const PiecewiseCurve& c, uint8_t x);