#include "PiecewiseCurve.h"

#include <algorithm>


//-----------------------------------------------------------
float evaluateCurveWeight(const PiecewiseCurve& c, uint8_t x)
//-----------------------------------------------------------
{
  auto normalize = [](float y) { return std::clamp(y / 100.0f, 0.0f, 1.0f); };

  if (x <= c.x0)
    return normalize(c.y0);

  if (x >= c.x1)
    return normalize(c.y1);

  if (c.x1 == c.x0)
    return normalize(c.y1);

  const float t = float(x - c.x0) / float(c.x1 - c.x0);
  const float y = float(c.y0) + t * float(int(c.y1) - int(c.y0));

  return normalize(y);
}