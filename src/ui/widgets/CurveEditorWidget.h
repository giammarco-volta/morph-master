#pragma once

#include <QWidget>
#include "../../core/PiecewiseCurve.h"
#include "../../core/KeyboardStandardRanges.h"


//--------------------------------------
class CurveEditorWidget : public QWidget
//--------------------------------------
{
  Q_OBJECT

public:
  explicit CurveEditorWidget(QWidget* parent, bool isOnlyForKey);

  void setCurve(const PiecewiseCurve& curve);
  PiecewiseCurve curve() const;
  void setKeyboardRange(KeyboardRangeId keyRange);

  void setShowXLabels(bool show);
  void setShowYLabels(bool show);

signals:
  void curveChanged(const PiecewiseCurve& curve);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  QPoint curveToPixel(int x, int y) const;
  QPoint pixelToCurve(const QPoint& p) const;
  QRect graphRect() const;
  uint8_t clampMidi(uint8_t v) const;
  QVector<int> visibleXMarks() const;

private:
  enum class DragTarget
  {
    None,
    P0,
    P1
  };

  PiecewiseCurve curve_;
  DragTarget dragTarget_ = DragTarget::None;

  bool isOnlyForKey_ = false;
  uint8_t minX_ = 0;
  uint8_t maxX_ = 127;

  bool showXLabels_ = true;
  bool showYLabels_ = true;
};