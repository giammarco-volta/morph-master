#include "CurveEditorTab.h"
#include "../widgets/MorphSurfaceWidget.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QScroller>
#include <QSizePolicy>
#include <QFrame>

//----------------------------------------------------------------------------------
CurveEditorTab::CurveEditorTab(QWidget* parent, bool isOnlyForKey) : QWidget(parent)
//----------------------------------------------------------------------------------
{
  auto* outerLayout = new QVBoxLayout(this);
  outerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->setSpacing(0);

  auto* scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  auto* content = new QWidget(scrollArea);
  content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  auto* root = new QVBoxLayout(content);
  root->setContentsMargins(6, 6, 6, 6);
  root->setSpacing(6);

  curveRows_[0] = new CurveRowWidget(isOnlyForKey ? tr("Curve N1") : tr("Curve V1"), this, isOnlyForKey);
  curveRows_[1] = new CurveRowWidget(isOnlyForKey ? tr("Curve N2") : tr("Curve V2"), this, isOnlyForKey);
  curveRows_[2] = new CurveRowWidget(isOnlyForKey ? tr("Curve N3") : tr("Curve V3"), this, isOnlyForKey);
  curveRows_[3] = new CurveRowWidget(isOnlyForKey ? tr("Curve N4") : tr("Curve V4"), this, isOnlyForKey);

  root->addWidget(curveRows_[0]);
  root->addWidget(curveRows_[1]);
  root->addWidget(curveRows_[2]);
  root->addWidget(curveRows_[3]);
  root->addStretch();

  scrollArea->setWidget(content);
  outerLayout->addWidget(scrollArea);

  PiecewiseCurve c1;
  PiecewiseCurve c2;
  PiecewiseCurve c3;
  PiecewiseCurve c4;

  if (isOnlyForKey)
  {
    c1.x0 = 36; c1.y0 =  0;  c1.x1 = 84; c1.y1 = 127;
    c2.x0 = 36; c2.y0 =  0;  c2.x1 = 72; c2.y1 = 127;
    c3.x0 = 48; c3.y0 =  0;  c3.x1 = 84; c3.y1 = 127;
    c4.x0 = 48; c4.y0 = 32;  c4.x1 = 72; c4.y1 =  96;
  }
  else
  {
    c1.x0 =  0; c1.y0 =  0;  c1.x1 = 127; c1.y1 = 127;
    c2.x0 = 32; c2.y0 =  0;  c2.x1 =  96; c2.y1 = 127;
    c3.x0 = 32; c3.y0 =  0;  c3.x1 =  64; c3.y1 = 127;
    c4.x0 = 48; c4.y0 = 32;  c4.x1 =  80; c4.y1 =  96;
  }

  curveRows_[0]->setCurve(c1);
  curveRows_[1]->setCurve(c2);
  curveRows_[2]->setCurve(c3);
  curveRows_[3]->setCurve(c4);
}

//--------------------------------------------------------------------
std::array<PiecewiseCurve, numOfCurves> CurveEditorTab::curves() const
//--------------------------------------------------------------------
{
  return {
    curveRows_[0]->curve(),
    curveRows_[1]->curve(),
    curveRows_[2]->curve(),
    curveRows_[3]->curve()
  };
}

//-----------------------------------------------------------------------------------
void CurveEditorTab::setCurves(const std::array<PiecewiseCurve, numOfCurves>& curves)
//-----------------------------------------------------------------------------------
{
  curveRows_[0]->setCurve(curves[0]);
  curveRows_[1]->setCurve(curves[1]);
  curveRows_[2]->setCurve(curves[2]);
  curveRows_[3]->setCurve(curves[3]);
}

//-------------------------------------------------------
void CurveEditorTab::setKeyboardRange(KeyboardRangeId kr)
//-------------------------------------------------------
{
  for (auto& c : curveRows_)
    c->setKeyboardRange(kr);
}