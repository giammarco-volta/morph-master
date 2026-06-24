#pragma once

#include <QWidget>
#include "../../core/PiecewiseCurve.h"
#include "../../core/KeyboardStandardRanges.h"

class QLabel;
class MidiValueSelector;
class CurveEditorWidget;


//-----------------------------------
class CurveRowWidget : public QWidget
//-----------------------------------
{
  Q_OBJECT

public:
  explicit CurveRowWidget(const QString& title, QWidget* parent, bool isOnlyForKey);

  void setCurve(const PiecewiseCurve& curve);
  PiecewiseCurve curve() const;
  void setKeyboardRange(KeyboardRangeId keyRange);

  void setShowXLabels(bool show);

signals:
  void curveChanged(const PiecewiseCurve& curve);

private slots:
  void onSpinChanged();
  void onEditorChanged(const PiecewiseCurve& curve);

private:
  void buildUi(const QString& title);
  void UpdateBoundaries();
  void syncUiFromCurve();

private:
  QLabel* titleLabel_ = nullptr;

  MidiValueSelector* x0Spin_ = nullptr;
  MidiValueSelector* y0Spin_ = nullptr;
  MidiValueSelector* x1Spin_ = nullptr;
  MidiValueSelector* y1Spin_ = nullptr;

  CurveEditorWidget* editor_ = nullptr;

  PiecewiseCurve curve_;

  bool isOnlyForKey_ = false;
  uint8_t minX_ = 0;
  uint8_t maxX_ = 127;
};