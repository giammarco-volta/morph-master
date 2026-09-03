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
int CurveController::x1() const
//-----------------------------
{
  const auto& preset = settingsController_.currentPreset();

  const auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  return curve.x0;
}

//------------------------------------
void CurveController::setX1(int value)
//------------------------------------
{
  auto& preset = settingsController_.currentPreset();

  auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  const int currentX2 = clampInt(curve.x1, minX() + 1, maxX());
  const int newValue = clampInt(value, minX(), currentX2 - 1);

  if (curve.x0 == newValue)
    return;

  curve.x0 = static_cast<uint8_t>(newValue);

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
int CurveController::x2() const
//-----------------------------
{
  const auto& preset = settingsController_.currentPreset();

  const auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  return curve.x1;
}

//------------------------------------
void CurveController::setX2(int value)
//------------------------------------
{
  auto& preset = settingsController_.currentPreset();

  auto& curve = curveSet_ == CurveSet::Key ? preset.keyCurve : preset.velCurve;

  const int currentX1 = clampInt(curve.x0, minX(), maxX() - 1);
  const int newValue = clampInt(value, currentX1 + 1, maxX());

  if (curve.x1 == newValue)
    return;

  curve.x1 = static_cast<uint8_t>(newValue);

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

  int newX1 = clampInt(curve.x0, minValue, maxValue - 1);
  int newX2 = clampInt(curve.x1, newX1 + 1, maxValue);

  const bool changed = curve.x0 != newX1 || curve.x1 != newX2;

  if (!changed)
    return false;

  curve.x0 = static_cast<uint8_t>(newX1);
  curve.x1 = static_cast<uint8_t>(newX2);

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