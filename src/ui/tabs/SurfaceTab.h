#pragma once

#include <QWidget>
#include "../../core/KeyboardStandardRanges.h"

class MorphSurfaceWidget;


//-------------------------------
class SurfaceTab : public QWidget
//-------------------------------
{
  Q_OBJECT

public:
  explicit SurfaceTab(QWidget* parent = nullptr);

  MorphSurfaceWidget* getMorphSurfaceWidget() const { return morphSurfaceWidget_; }

  void onNoteOn(uint8_t note, uint8_t velocity);
  void onNoteOff(uint8_t note);

private:
  MorphSurfaceWidget* morphSurfaceWidget_{};
};