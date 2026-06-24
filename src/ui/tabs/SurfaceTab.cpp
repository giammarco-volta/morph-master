#include "SurfaceTab.h"

#include <QVBoxLayout>
#include "../widgets/MorphSurfaceWidget.h"


//-------------------------------------------------------
SurfaceTab::SurfaceTab(QWidget* parent) : QWidget(parent)
//-------------------------------------------------------
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  morphSurfaceWidget_ = new MorphSurfaceWidget(this);
  morphSurfaceWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  layout->addWidget(morphSurfaceWidget_);
}

//-------------------------------------------------------
void SurfaceTab::onNoteOn(uint8_t note, uint8_t velocity)
//-------------------------------------------------------
{
  morphSurfaceWidget_->onNoteOn(note, velocity);
}

//--------------------------------------
void SurfaceTab::onNoteOff(uint8_t note)
//--------------------------------------
{
  morphSurfaceWidget_->onNoteOff(note);
}
