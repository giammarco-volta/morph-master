#include "CurveRowWidget.h"

#include "CurveEditorWidget.h"
#include "../../../../Common/src/ui/widgets/MidiValueSelector.h"

#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSignalBlocker>


//-------------------------------------------------------------------------------------------------------------------------------------
CurveRowWidget::CurveRowWidget(const QString& title, QWidget* parent, bool isOnlyForKey) : QWidget(parent), isOnlyForKey_(isOnlyForKey)
//-------------------------------------------------------------------------------------------------------------------------------------
{
  buildUi(title);
  syncUiFromCurve();
}

//------------------------------------------------
void CurveRowWidget::buildUi(const QString& title)
//------------------------------------------------
{
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(4, 4, 4, 4);
  root->setSpacing(4);

  titleLabel_ = new QLabel(title, this);
  root->addWidget(titleLabel_);

  auto* row = new QHBoxLayout();
  row->setSpacing(8);

  auto* leftGrid = new QGridLayout();
  leftGrid->setHorizontalSpacing(4);
  leftGrid->setVerticalSpacing(2);

  x0Spin_ = new MidiValueSelector(this);
  y0Spin_ = new MidiValueSelector(this);
  x1Spin_ = new MidiValueSelector(this);
  y1Spin_ = new MidiValueSelector(this);

  x0Spin_->setRange(0, 126);
  x1Spin_->setRange(1, 127);
  y0Spin_->setRange(0, 127);
  y1Spin_->setRange(0, 127);

  leftGrid->addWidget(new QLabel("x1", this), 0, 0);
  leftGrid->addWidget(x0Spin_, 0, 1);
  leftGrid->addWidget(new QLabel("y1", this), 0, 2);
  leftGrid->addWidget(y0Spin_, 0, 3);

  leftGrid->addWidget(new QLabel("x2", this), 1, 0);
  leftGrid->addWidget(x1Spin_, 1, 1);
  leftGrid->addWidget(new QLabel("y2", this), 1, 2);
  leftGrid->addWidget(y1Spin_, 1, 3);

  editor_ = new CurveEditorWidget(this, isOnlyForKey_);

  row->addLayout(leftGrid, 0);
  row->addWidget(editor_, 1);

  root->addLayout(row);

  connect(x0Spin_, &MidiValueSelector::valueChanged, this, &CurveRowWidget::onSpinChanged);
  connect(y0Spin_, &MidiValueSelector::valueChanged, this, &CurveRowWidget::onSpinChanged);
  connect(x1Spin_, &MidiValueSelector::valueChanged, this, &CurveRowWidget::onSpinChanged);
  connect(y1Spin_, &MidiValueSelector::valueChanged, this, &CurveRowWidget::onSpinChanged);

  connect(editor_, &CurveEditorWidget::curveChanged, this, &CurveRowWidget::onEditorChanged);
}

//--------------------------------------------------------
void CurveRowWidget::setCurve(const PiecewiseCurve& curve)
//--------------------------------------------------------
{
  curve_ = curve;
  syncUiFromCurve();
}

//------------------------------------------
PiecewiseCurve CurveRowWidget::curve() const
//------------------------------------------
{
  return curve_;
}

//--------------------------------------------
void CurveRowWidget::setShowXLabels(bool show)
//--------------------------------------------
{
  editor_->setShowXLabels(show);
}

//-------------------------------------
void CurveRowWidget::UpdateBoundaries()
//-------------------------------------
{
}

//-------------------------------------------------------------
void CurveRowWidget::setKeyboardRange(KeyboardRangeId keyRange)
//-------------------------------------------------------------
{
  if (!isOnlyForKey_)
    return;

  const KeyboardRange& r = kKeyboardRanges[static_cast<uint8_t>(keyRange)];

  minX_ = r.minNote;
  maxX_ = r.maxNote;

  uint8_t valueX0 = std::min(std::max((uint8_t)x0Spin_->value(), minX_), maxX_);
  uint8_t valueX1 = std::min(std::max((uint8_t)x1Spin_->value(), minX_), maxX_);

  x0Spin_->setRange(minX_, maxX_ - 1);
  x1Spin_->setRange(minX_ + 1, maxX_);

  x0Spin_->setValue(valueX0);
  x1Spin_->setValue(valueX1);

  if (curve_.x0 < minX_)
    curve_.x0 = minX_;

  if (curve_.x1 > maxX_)
    curve_.x1 = maxX_;

  if (curve_.x0 >= curve_.x1)
    curve_.x1 = std::min(maxX_, (uint8_t)(curve_.x0 + 1));

  syncUiFromCurve();

  editor_->setKeyboardRange(keyRange);
}

//------------------------------------
void CurveRowWidget::syncUiFromCurve()
//------------------------------------
{
  QSignalBlocker b0(x0Spin_);
  QSignalBlocker b1(y0Spin_);
  QSignalBlocker b2(x1Spin_);
  QSignalBlocker b3(y1Spin_);

  x0Spin_->setValue(curve_.x0);
  y0Spin_->setValue(curve_.y0);
  x1Spin_->setValue(curve_.x1);
  y1Spin_->setValue(curve_.y1);

  editor_->setCurve(curve_);
}

//----------------------------------
void CurveRowWidget::onSpinChanged()
//----------------------------------
{
  curve_.x0 = x0Spin_->value();
  curve_.y0 = y0Spin_->value();
  curve_.x1 = x1Spin_->value();
  curve_.y1 = y1Spin_->value();

  if (curve_.x0 >= curve_.x1)
  {
    curve_.x1 = std::min(127, curve_.x0 + 1);
  }

  syncUiFromCurve();
  emit curveChanged(curve_);
}

//---------------------------------------------------------------
void CurveRowWidget::onEditorChanged(const PiecewiseCurve& curve)
//---------------------------------------------------------------
{
  curve_ = curve;
  syncUiFromCurve();
  emit curveChanged(curve_);
}