#include "ExpressionCalculator.h"
#include <algorithm>
#include <cmath>


//------------------------------------------------------------------------------------------
double CurveCalculator::computeScaleFactor( const MorphOutputProfile& gp,
                                            const PiecewiseCurve& keyCurve,
                                            const PiecewiseCurve& velCurve,
                                            uint8_t note,
                                            uint8_t velocity)
//------------------------------------------------------------------------------------------
{
  static constexpr double kHalfPi = 1.5707963267948966;

  float keyExprWeight = evaluateCurveWeight(keyCurve, note);
  float velExprWeight = evaluateCurveWeight(velCurve, velocity);

  double keyExprNorm = gp.invertKey      ? std::cos(keyExprWeight * kHalfPi) : std::sin(keyExprWeight * kHalfPi);
  double velExprNorm = gp.invertVelocity ? std::cos(velExprWeight * kHalfPi) : std::sin(velExprWeight * kHalfPi);

  if (!gp.useKey)
    return velExprNorm;

  if (!gp.useVelocity)
    return keyExprNorm;

  return keyExprNorm * velExprNorm;
}

//------------------------------------------------------------------------------------------
uint8_t CurveCalculator::WeightTheValue(const MorphOutputProfile& gp,
                                        const PiecewiseCurve& keyCurve,
                                        const PiecewiseCurve& velCurve,
                                        uint8_t note,
                                        uint8_t velocity,
                                        uint8_t value)
//------------------------------------------------------------------------------------------
{
  double r = computeScaleFactor(gp, keyCurve, velCurve, note, velocity);
  return std::clamp(uint8_t(std::lround(double(value) * r)), (uint8_t)0, (uint8_t)127);
}
