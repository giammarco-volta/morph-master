#include "ExpressionCalculator.h"
#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t ExpressionCalculator::evaluateSelectedCurve(ExpressionCurveId id,
                                                    const std::array<PiecewiseCurve, 4>& keyCurves,
                                                    const std::array<PiecewiseCurve, 4>& velCurves,
                                                    uint8_t x,
                                                    uint8_t defaultValue)
//-----------------------------------------------------------------------------------------------------------------------------------------------------
{
  switch (id)
  {
  case ExpressionCurveId::CurveH1: return evaluateCurve(keyCurves[0], x);
  case ExpressionCurveId::CurveH2: return evaluateCurve(keyCurves[1], x);
  case ExpressionCurveId::CurveH3: return evaluateCurve(keyCurves[2], x);
  case ExpressionCurveId::CurveH4: return evaluateCurve(keyCurves[3], x);
  case ExpressionCurveId::CurveV1: return evaluateCurve(velCurves[0], x);
  case ExpressionCurveId::CurveV2: return evaluateCurve(velCurves[1], x);
  case ExpressionCurveId::CurveV3: return evaluateCurve(velCurves[2], x);
  case ExpressionCurveId::CurveV4: return evaluateCurve(velCurves[3], x);
  }

  return defaultValue;
}

//------------------------------------------------------------------------------------------
uint8_t ExpressionCalculator::computeExpression(ExpressionCurveId keyCurveId,
                                                ExpressionCurveId velCurveId,
                                                GroupMorphProfile gp,
                                                const std::array<PiecewiseCurve, 4>& keyCurves,
                                                const std::array<PiecewiseCurve, 4>& velCurves,
                                                uint8_t note,
                                                uint8_t velocity)
//------------------------------------------------------------------------------------------
{
  double r = computeScaleFactor(keyCurveId, velCurveId, gp, keyCurves, velCurves, note, velocity);
  return std::clamp(uint8_t(std::lround(127.0 * r)), (uint8_t)0, (uint8_t)127);
}

//------------------------------------------------------------------------------------------
double ExpressionCalculator::computeScaleFactor(ExpressionCurveId keyCurveId,
                                                ExpressionCurveId velCurveId,
                                                GroupMorphProfile gp,
                                                const std::array<PiecewiseCurve, 4>& keyCurves,
                                                const std::array<PiecewiseCurve, 4>& velCurves,
                                                uint8_t note,
                                                uint8_t velocity)
//------------------------------------------------------------------------------------------
{
  static constexpr double kHalfPi = 1.5707963267948966;

  uint8_t keyExpr = evaluateSelectedCurve(keyCurveId, keyCurves, velCurves, note, 64);
  uint8_t velExpr = evaluateSelectedCurve(velCurveId, keyCurves, velCurves, velocity, 64);

  double pkey = double(keyExpr) / 127.0;      // 0..1
  double pvel = double(velExpr) / 127.0;      // 0..1

  double keyExprNorm = gp.invertKey      ? std::cos(pkey * kHalfPi) : std::sin(pkey * kHalfPi);
  double velExprNorm = gp.invertVelocity ? std::cos(pvel * kHalfPi) : std::sin(pvel * kHalfPi);

  if (!gp.useKey)
    return velExprNorm;

  else if (!gp.useVelocity)
    return keyExprNorm;

  return std::sqrt((keyExprNorm * keyExprNorm + velExprNorm * velExprNorm) / 2.0);
}