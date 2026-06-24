#include "CurveEditorWidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
CurveEditorWidget::CurveEditorWidget(QWidget* parent, bool isOnlyForKey) : QWidget(parent), isOnlyForKey_(isOnlyForKey)
//---------------------------------------------------------------------------------------------------------------------
{
  setMinimumSize(260, 120);
  setMouseTracking(true);
}

//-----------------------------------------------------------
void CurveEditorWidget::setCurve(const PiecewiseCurve& curve)
//-----------------------------------------------------------
{
  curve_ = curve;
  update();
}

//---------------------------------------------
PiecewiseCurve CurveEditorWidget::curve() const
//---------------------------------------------
{
  return curve_;
}
 
//----------------------------------------------------------------
void CurveEditorWidget::setKeyboardRange(KeyboardRangeId keyRange)
//----------------------------------------------------------------
{
  if (!isOnlyForKey_)
    return;

  const KeyboardRange& r = kKeyboardRanges[static_cast<uint8_t>(keyRange)];

  minX_ = r.minNote;
  maxX_ = r.maxNote;

  if (curve_.x0 < minX_)
    curve_.x0 = minX_;

  if (curve_.x1 > maxX_)
    curve_.x1 = maxX_;

  if (curve_.x0 >= curve_.x1)
    curve_.x1 = std::min(maxX_, (uint8_t)(curve_.x0 + 1));

  update();
}

//---------------------------------------------------
uint8_t CurveEditorWidget::clampMidi(uint8_t v) const
//---------------------------------------------------
{
  return std::max((uint8_t)0, std::min((uint8_t)127, v));
}

//----------------------------------------
QRect CurveEditorWidget::graphRect() const
//----------------------------------------
{
  const int leftMargin = 12;
  const int topMargin = showXLabels_ ? 24 : 12;
  const int rightMargin = showYLabels_ ? 36 : 12;
  const int bottomMargin = 12;

  return rect().adjusted(leftMargin, topMargin, -rightMargin, -bottomMargin);
}

//---------------------------------------------------
QVector<int> CurveEditorWidget::visibleXMarks() const
//---------------------------------------------------
{
  QVector<int> marks;

  if (isOnlyForKey_)
  {
    static constexpr int baseMarks[] = { 12, 24, 36, 48, 60, 72, 84, 96, 108, 120 };
    for (int v : baseMarks)
      if (v >= minX_ && v <= maxX_)
        marks.push_back(v);
  }
  else
  {
    auto addIfMissing = [&marks](int v)
      {
        if (!marks.contains(v))
          marks.push_back(v);
      };

    addIfMissing(minX_);

    static constexpr int baseMarks[] = { 0, 32, 64, 96, 127 };
    for (int v : baseMarks)
    {
      if (v >= minX_ && v <= maxX_)
        addIfMissing(v);
    }

    addIfMissing(maxX_);
  }
  std::sort(marks.begin(), marks.end());
  return marks;
}

//--------------------------------------------------------
QPoint CurveEditorWidget::curveToPixel(int x, int y) const
//--------------------------------------------------------
{
  const QRect r = graphRect();

  const double xNorm = double(x - minX_) / double(maxX_ - minX_);
  const double px = r.left() + xNorm * r.width();
  const double py = r.bottom() - (double(y) / 127.0) * r.height();

  return QPoint(int(std::lround(px)), int(std::lround(py)));
}

//-----------------------------------------------------------
QPoint CurveEditorWidget::pixelToCurve(const QPoint& p) const
//-----------------------------------------------------------
{
  const QRect r = graphRect();

  const int px = std::max(r.left(), std::min(r.right(), p.x()));
  const int py = std::max(r.top(), std::min(r.bottom(), p.y()));

  const double xNorm = double(px - r.left()) / double(r.width());
  const double yNorm = double(r.bottom() - py) / double(r.height());

  const int x = int(std::lround(minX_ + xNorm * double(maxX_ - minX_)));
  const int y = int(std::lround(yNorm * 127.0));

  return QPoint(clampMidi(x), clampMidi(y));
}

//-----------------------------------------------
void CurveEditorWidget::setShowXLabels(bool show)
//-----------------------------------------------
{
  showXLabels_ = show;
  update();
}

//-----------------------------------------------
void CurveEditorWidget::setShowYLabels(bool show)
//-----------------------------------------------
{
  showYLabels_ = show;
  update();
}

//--------------------------------------------------------
void CurveEditorWidget::paintEvent(QPaintEvent* /*event*/)
//--------------------------------------------------------
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const QRect r = graphRect();

  p.fillRect(rect(), palette().window());
  p.setPen(QPen(Qt::gray, 1));
  p.drawRect(r);

  const QVector<int> xMarks = visibleXMarks();
  const int yMarks[] = { 0, 32, 64, 96, 127 };

  // Griglia
  p.setPen(QPen(QColor(180, 180, 180), 1, Qt::DotLine));

  for (int xMark : xMarks)
  {
    const QPoint pt = curveToPixel(xMark, 0);
    p.drawLine(pt.x(), r.top(), pt.x(), r.bottom());
  }

  for (int yMark : yMarks)
  {
    const QPoint pt = curveToPixel(minX_, yMark);
    p.drawLine(r.left(), pt.y(), r.right(), pt.y());
  }

  // Curva
  const QPoint pStart = curveToPixel(minX_, curve_.y0);
  const QPoint p0 = curveToPixel(curve_.x0, curve_.y0);
  const QPoint p1 = curveToPixel(curve_.x1, curve_.y1);
  const QPoint pEnd = curveToPixel(maxX_, curve_.y1);

  p.setPen(QPen(Qt::darkYellow, 2));
  p.drawLine(pStart, p0);
  p.drawLine(p0, p1);
  p.drawLine(p1, pEnd);

  p.setBrush(Qt::white);
  p.setPen(QPen(Qt::darkYellow, 2));
  p.drawEllipse(p0, 5, 5);
  p.drawEllipse(p1, 5, 5);

  // Curva complementare
  const QPoint pStart2 = curveToPixel(minX_, 127 - curve_.y0);
  const QPoint p02 = curveToPixel(curve_.x0, 127 - curve_.y0);
  const QPoint p12 = curveToPixel(curve_.x1, 127 - curve_.y1);
  const QPoint pEnd2 = curveToPixel(maxX_, 127 - curve_.y1);

  p.setPen(QPen(Qt::darkYellow, 2, Qt::DotLine));
  p.drawLine(pStart2, p02);
  p.drawLine(p02, p12);
  p.drawLine(p12, pEnd2);

  // Etichette X sopra il grafico
  if (showXLabels_)
  {
    p.setPen(palette().color(QPalette::WindowText));

    for (int xMark : xMarks)
    {
      const QPoint pt = curveToPixel(xMark, 127);
      QRect textRect(pt.x() - 16, 2, 32, r.top() - 4);
      p.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, isOnlyForKey_ ? "C" + QString::number((xMark - 12) / 12) : QString::number(xMark));
    }
  }

  // Etichette Y a destra del grafico
  if (showYLabels_)
  {
    p.setPen(palette().color(QPalette::WindowText));

    for (int yMark : yMarks)
    {
      const QPoint pt = curveToPixel(maxX_, yMark);
      QRect textRect(r.right() + 6, pt.y() - 10, 28, 20);
      p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, QString::number(yMark));
    }
  }
}

//---------------------------------------------------------
void CurveEditorWidget::mousePressEvent(QMouseEvent* event)
//---------------------------------------------------------
{
  const QPoint p0 = curveToPixel(curve_.x0, curve_.y0);
  const QPoint p1 = curveToPixel(curve_.x1, curve_.y1);

  const int d0 = (event->pos() - p0).manhattanLength();
  const int d1 = (event->pos() - p1).manhattanLength();

  constexpr int hitRadius = 10;

  if (d0 <= hitRadius && d0 <= d1)
    dragTarget_ = DragTarget::P0;
  else if (d1 <= hitRadius)
    dragTarget_ = DragTarget::P1;
  else
    dragTarget_ = DragTarget::None;
}

//--------------------------------------------------------
void CurveEditorWidget::mouseMoveEvent(QMouseEvent* event)
//--------------------------------------------------------
{
  if (dragTarget_ == DragTarget::None)
    return;

  const QPoint c = pixelToCurve(event->pos());

  if (dragTarget_ == DragTarget::P0)
  {
    curve_.x0 = std::min<int>(c.x(), curve_.x1 - 1);
    curve_.x0 = std::max<int>(curve_.x0, minX_);
    curve_.y0 = clampMidi(c.y());
  }
  else if (dragTarget_ == DragTarget::P1)
  {
    curve_.x1 = std::max<int>(c.x(), curve_.x0 + 1);
    curve_.x1 = std::min<int>(curve_.x1, maxX_);
    curve_.y1 = clampMidi(c.y());
  }

  update();
  emit curveChanged(curve_);
}

//---------------------------------------------------------------
void CurveEditorWidget::mouseReleaseEvent(QMouseEvent* /*event*/)
//---------------------------------------------------------------
{
  dragTarget_ = DragTarget::None;
}