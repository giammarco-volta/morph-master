#include "CurveController.h"

#include "SettingsController.h"
#include "../core/PiecewiseCurve.h"
#include "../core/KeyboardStandardRanges.h"

#include <algorithm>

namespace
{
  //--------------------------------------------------------
  const KeyboardRange* findKeyboardRange(KeyboardRangeId id)
  //--------------------------------------------------------
  {
    for (const auto& range : kKeyboardRanges)
      if (range.id == id)
        return &range;

    return &kKeyboardRanges[0];
  }

  //-------------------------------------
  int clampInt(int value, int lo, int hi)
  //-------------------------------------
  {
    return std::max(lo, std::min(value, hi));
  }
}

//-------------------------------------------------------------------------
CurveController::CurveController( SettingsController& settingsController,
                                  CurveSet curveSet,
                                  QObject* parent)
                                  : QObject(parent)
                                  , settingsController_(settingsController)
                                  , curveSet_(curveSet)
//-------------------------------------------------------------------------
{
  connect(&settingsController_, &SettingsController::curveControlValuesChanged, this, &CurveController::effectiveCurveChanged);
  connect(&settingsController_, &SettingsController::curveControlsChanged, this, &CurveController::effectiveCurveChanged);
  connect(this, &CurveController::x1Changed, this, &CurveController::effectiveCurveChanged);
  connect(this, &CurveController::x2Changed, this, &CurveController::effectiveCurveChanged);
  connect(this, &CurveController::y1Changed, this, &CurveController::effectiveCurveChanged);
  connect(this, &CurveController::y2Changed, this, &CurveController::effectiveCurveChanged);
  if (curveSet_ == CurveSet::Key)
  {
    connect(&settingsController_,
      &SettingsController::keyboardRangeIdChanged,
      this,
      [this]()
      {
        const bool curveChanged = normalizeXToCurrentRange();

        emit rangeChanged();

        if (curveChanged)
        {
          emit x1Changed();
          emit x2Changed();
        }
      });
  }
}

//------------------------------------
QString CurveController::title() const
//------------------------------------
{
  return curveSet_ == CurveSet::Key
    ? QStringLiteral("Low and High Morph Curves")
    : QStringLiteral("Soft and Loud Morph Curves");
}

//------------------------------------
bool CurveController::noteMode() const
//------------------------------------
{
  return curveSet_ == CurveSet::Key;
}

//-------------------------------
int CurveController::minX() const
//-------------------------------
{
  if (curveSet_ == CurveSet::Velocity)
    return 0;

  const auto id = static_cast<KeyboardRangeId>(settingsController_.keyboardRangeId());

  return findKeyboardRange(id)->minNote;
}

//-------------------------------
int CurveController::maxX() const
//-------------------------------
{
  if (curveSet_ == CurveSet::Velocity)
    return 127;

  const auto id = static_cast<KeyboardRangeId>(settingsController_.keyboardRangeId());

  return findKeyboardRange(id)->maxNote;
}

//-----------------------------
double CurveController::effectiveX1() const
{
  return settingsController_.effectiveCurveTransform(curveSet_==CurveSet::Key ? 0 : 1).outputFor(x1());
}
double CurveController::effectiveX2() const
{
  return settingsController_.effectiveCurveTransform(curveSet_==CurveSet::Key ? 0 : 1).outputFor(x2());
}

double CurveController::x1() const
//-----------------------------
{
  const auto& preset = settingsController_.currentPreset();

  const auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  return curve.x0;
}

//------------------------------------
void CurveController::setX1(double value)
//------------------------------------
{
  auto& preset = settingsController_.currentPreset();

  auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  const double currentX2 = std::clamp(curve.x1, double(minX()+1), double(maxX()));
  const double newValue = std::clamp(value, double(minX()), currentX2-1);

  if (curve.x0 == newValue)
    return;

  curve.x0 = newValue;

  emit x1Changed();

  settingsController_.notifyMidiRelevantStateChanged();
}

//-----------------------------
int CurveController::y1() const
//-----------------------------
{
  const auto& preset = settingsController_.currentPreset();

  const auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  return curve.y0;
}

//------------------------------------
void CurveController::setY1(int value)
//------------------------------------
{
  auto& preset = settingsController_.currentPreset();

  auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  const auto newValue = clampInt(value, 0, 100);

  if (curve.y0 == newValue)
    return;

  curve.y0 = newValue;

  emit y1Changed();

  settingsController_.notifyMidiRelevantStateChanged();
}
//-----------------------------
double CurveController::x2() const
//-----------------------------
{
  const auto& preset = settingsController_.currentPreset();

  const auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  return curve.x1;
}

//------------------------------------
void CurveController::setX2(double value)
//------------------------------------
{
  auto& preset = settingsController_.currentPreset();

  auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  const double currentX1 = std::clamp(curve.x0, double(minX()), double(maxX()-1));
  const double newValue = std::clamp(value, currentX1+1, double(maxX()));

  if (curve.x1 == newValue)
    return;

  curve.x1 = newValue;

  emit x2Changed();

  settingsController_.notifyMidiRelevantStateChanged();
}

//-----------------------------
int CurveController::y2() const
//-----------------------------
{
  const auto& preset = settingsController_.currentPreset();

  const auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  return curve.y1;
}

//------------------------------------
void CurveController::setY2(int value)
//------------------------------------
{
  auto& preset = settingsController_.currentPreset();

  auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  const auto newValue = clampInt(value, 0, 100);

  if (curve.y1 == newValue)
    return;

  curve.y1 = newValue;

  emit y2Changed();

  settingsController_.notifyMidiRelevantStateChanged();
}

//----------------------------------------------
bool CurveController::normalizeXToCurrentRange()
//----------------------------------------------
{
  if (curveSet_ != CurveSet::Key)
    return false;

  auto& preset = settingsController_.currentPreset();
  auto& curve = preset.keyCurve;

  const int minValue = minX();
  const int maxValue = maxX();

  double newX1 = std::clamp(curve.x0, double(minValue), double(maxValue-1));
  double newX2 = std::clamp(curve.x1, newX1+1, double(maxValue));

  const bool changed = curve.x0 != newX1 || curve.x1 != newX2;

  if (!changed)
    return false;

  curve.x0 = newX1;
  curve.x1 = newX2;

  return true;
}

//---------------------------------------
void CurveController::notifyDataChanged()
//---------------------------------------
{
  emit rangeChanged();

  emit x1Changed();
  emit y1Changed();
  emit x2Changed();
  emit y2Changed();
}
void CurveController::setPoints(double firstX, int firstY, double lastX, int lastY)
{
  if(!std::isfinite(firstX) || !std::isfinite(lastX)) return;
  auto& preset=settingsController_.currentPreset();
  auto& curve=curveSet_==CurveSet::Key ? preset.keyCurve : preset.velCurve;
  curve.x0=std::clamp(firstX,double(minX()),double(maxX()-1));
  curve.x1=std::clamp(lastX,curve.x0+1,double(maxX()));
  curve.y0=std::clamp(firstY,0,100);
  curve.y1=std::clamp(lastY,0,100);
  notifyDataChanged();
  settingsController_.notifyMidiRelevantStateChanged();
}

void CurveController::notifyHorizontalChanged()
{
  emit x1Changed();
  emit x2Changed();
}
