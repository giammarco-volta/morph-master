#pragma once

#include "PiecewiseCurve.h"
#include "ExpressionCurveId.h"
#include "TrackGroupId.h"
#include <array>

class ExpressionCalculator
{
public:
  static uint8_t evaluateSelectedCurve( ExpressionCurveId id,
                                        const std::array<PiecewiseCurve, 4>& keyCurves,
                                        const std::array<PiecewiseCurve, 4>& velCurves,
                                        uint8_t x,
                                        uint8_t defaultValue = 64);

  static uint8_t computeExpression( ExpressionCurveId keyCurveId,
                                    ExpressionCurveId velCurveId,
                                    GroupMorphProfile gp,
                                    const std::array<PiecewiseCurve, 4>& keyCurves,
                                    const std::array<PiecewiseCurve, 4>& velCurves,
                                    uint8_t note,
                                    uint8_t velocity);
  static double computeScaleFactor( ExpressionCurveId keyCurveId,
                                    ExpressionCurveId velCurveId,
                                    GroupMorphProfile gp,
                                    const std::array<PiecewiseCurve, 4>& keyCurves,
                                    const std::array<PiecewiseCurve, 4>& velCurves,
                                    uint8_t note,
                                    uint8_t velocity);
};