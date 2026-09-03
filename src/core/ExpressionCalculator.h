#pragma once

#include "PiecewiseCurve.h"
#include "MorphOutputId.h"
#include <array>

//-------------------
class CurveCalculator
//-------------------
{
public:
  static double computeScaleFactor(const MorphOutputProfile& gp,
                                   const PiecewiseCurve& keyCurve,
                                   const PiecewiseCurve& velCurve,
                                   uint8_t note,
                                   uint8_t velocity);

  static uint8_t WeightTheValue(const MorphOutputProfile& gp,
                                const PiecewiseCurve& keyCurve,
                                const PiecewiseCurve& velCurve,
                                uint8_t note,
                                uint8_t velocity,
                                uint8_t value);
};