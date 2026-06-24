#include "PiecewiseCurve.h"

#include <cmath>

uint8_t evaluateCurve(const PiecewiseCurve& c, uint8_t x)
{
  if (x <= c.x0)
    return c.y0;

  if (x >= c.x1)
    return c.y1;

  if (c.x1 == c.x0)
    return c.y1;

  const float t = float(x - c.x0) / float(c.x1 - c.x0);
  return uint8_t(std::lround(c.y0 + t * (c.y1 - c.y0)));
}