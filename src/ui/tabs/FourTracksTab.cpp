#include "FourTracksTab.h"

#include "../widgets/TrackStripWidget.h"
#include "../../MainWindow.h"
#include "../../core/Presets.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScroller>
#include <QSizePolicy>
#include <QFrame>

//----------------------------------------------------------------------------------------------------------------
FourTracksTab::FourTracksTab(MainWindow* parent, uint8_t trackOffset) : QWidget(parent), trackOffset_(trackOffset)
//----------------------------------------------------------------------------------------------------------------
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
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  grid_ = new QGridLayout();
  grid_->setContentsMargins(8, 8, 8, 8);
  grid_->setSpacing(8);

  root->addLayout(grid_);
  root->addStretch();

  scrollArea->setWidget(content);
  outerLayout->addWidget(scrollArea);

  createTrackWidgets(parent);

  connect(this, &FourTracksTab::instrumentProgramSelected, parent, &MainWindow::onInstrumentProgramSelected);
  connect(this, &FourTracksTab::manualProgramSelected, parent, &MainWindow::onManualProgramSelected);
}

//------------------------------------------------------------
void FourTracksTab::createTrackWidgets(MainWindow* mainWindow)
//------------------------------------------------------------
{
  constexpr int kCols = 4;

  for (uint8_t loctrk = 0; loctrk < kTrackCount; ++loctrk)
  {
    const uint8_t abstrk = uint8_t(trackOffset_ + loctrk);

    auto* strip = new TrackStripWidget(this, abstrk);

    strip->setPatchPolicy(patchPolicy_);
    if (instrumentDefinition_)
      strip->setInstrumentDefinition(instrumentDefinition_);

    connect(strip, &TrackStripWidget::trackFootageChanged, mainWindow, &MainWindow::onTrackFootageChanged);
    connect(strip, &TrackStripWidget::trackProgramChanged, mainWindow, &MainWindow::onTrackProgramChanged);
    connect(strip, &TrackStripWidget::trackVolumeChanged, mainWindow, &MainWindow::onTrackVolumeChanged);
    connect(strip, &TrackStripWidget::trackPanChanged, mainWindow, &MainWindow::onTrackPanChanged);
    connect(strip, &TrackStripWidget::trackReverbChanged, mainWindow, &MainWindow::onTrackReverbChanged);
    connect(strip, &TrackStripWidget::trackChorusChanged, mainWindow, &MainWindow::onTrackChorusChanged);
    connect(strip, &TrackStripWidget::trackTimbre1Changed, mainWindow, &MainWindow::onTrackTimbre1Changed);
    connect(strip, &TrackStripWidget::trackTimbre2Changed, mainWindow, &MainWindow::onTrackTimbre2Changed);

    // Se TrackStripWidget espone questi segnali, conviene collegarli qui.
    // In caso contrario puoi rimuovere queste due connect: i rispettivi slot restano innocui.
    connect(strip, &TrackStripWidget::instrumentProgramSelected, this, &FourTracksTab::onInstrumentProgramSelected);
    connect(strip, &TrackStripWidget::manualProgramSelected, this, &FourTracksTab::onManualProgramSelected);

    trackWidgets_[loctrk] = strip;

    const int row = loctrk / kCols;
    const int col = loctrk % kCols;
    grid_->addWidget(strip, row, col, Qt::AlignTop);
  }

  // Do not stretch the row vertically: inactive strips should keep their compact height.
  grid_->setRowStretch(0, 0);
}

//--------------------------------------------------------------------
TrackGroupId FourTracksTab::getGroupIdByAbsTrack(uint8_t abstrk) const
//--------------------------------------------------------------------
{
  return getGroupIdByRelTrack(abstrk - trackOffset_);
}

//--------------------------------------------------------------------
TrackGroupId FourTracksTab::getGroupIdByRelTrack(uint8_t loctrk) const
//--------------------------------------------------------------------
{
  assert(loctrk < kTrackCount);
  return (loctrk < kTrackCount) ? trackWidgets_[loctrk]->groupId() : TrackGroupId::None;
}

//----------------------------------------------------------------------------
void FourTracksTab::removeTrackFromGroup(uint8_t abstrk, TrackGroupId groupId)
//----------------------------------------------------------------------------
{
  uint8_t loctrk = uint8_t(abstrk - trackOffset_);
  if (loctrk < kTrackCount)
    trackWidgets_[loctrk]->setGroupId(TrackGroupId::None);
}

//--------------------------------------------------------------------------
void FourTracksTab::assignTrackToGroup(uint8_t abstrk, TrackGroupId groupId)
//--------------------------------------------------------------------------
{
  uint8_t loctrk = uint8_t(abstrk - trackOffset_);
  if (loctrk < kTrackCount)
    trackWidgets_[loctrk]->setGroupId(groupId);
}

//------------------------------------------------------
std::vector<uint8_t> FourTracksTab::trackIndices() const
//------------------------------------------------------
{
  std::vector<uint8_t> v;
  v.reserve(kTrackCount);

  for (uint8_t i = 0; i < kTrackCount; ++i)
    if (trackWidgets_[i]->isActive())
      v.push_back(uint8_t(trackOffset_ + i));

  return v;
}

//----------------------------------------------------------------
TrackStripWidget* FourTracksTab::trackWidget(uint8_t abstrk) const
//----------------------------------------------------------------
{
  if (abstrk < trackOffset_)
    return nullptr;

  const uint8_t loctrk = uint8_t(abstrk - trackOffset_);
  if (loctrk >= kTrackCount)
    return nullptr;

  return trackWidgets_[loctrk];
}

//------------------------------------------------------------------
ExpressionCurveId FourTracksTab::keyExprCurveId(uint8_t abstrk) const
//------------------------------------------------------------------
{
  if (auto* w = trackWidget(abstrk))
    return w->keyExprCurveId();
  return ExpressionCurveId();
}

//------------------------------------------------------------------
ExpressionCurveId FourTracksTab::velExprCurveId(uint8_t abstrk) const
//------------------------------------------------------------------
{
  if (auto* w = trackWidget(abstrk))
    return w->velExprCurveId();
  return ExpressionCurveId();
}

//-----------------------------------------------------
int8_t FourTracksTab::timbre1Value(uint8_t abstrk) const
//-----------------------------------------------------
{
  if (auto* w = trackWidget(abstrk))
    return w->timbre1Value();
  return 0;
}

//-----------------------------------------------------
int8_t FourTracksTab::timbre2Value(uint8_t abstrk) const
//-----------------------------------------------------
{
  if (auto* w = trackWidget(abstrk))
    return w->timbre2Value();
  return 0;
}

//------------------------------------------------------
void FourTracksTab::resetTrackMidiWidgets(uint8_t abstrk)
//------------------------------------------------------
{
  if (auto* w = trackWidget(abstrk))
    w->resetTrackMidiWidgets();
}

//-----------------------------------------------------------------------------------
void FourTracksTab::setTrackPresetData(const TrackPresetData& preset, uint8_t abstrk)
//-----------------------------------------------------------------------------------
{
  if (auto* w = trackWidget(abstrk))
    w->setTrackPresetData(preset);
}

//-----------------------------------------------------------------------------------
void FourTracksTab::getTrackPresetData(TrackPresetData& preset, uint8_t abstrk) const
//-----------------------------------------------------------------------------------
{
  if (auto* w = trackWidget(abstrk))
    w->getTrackPresetData(preset);
}

//---------------------------------------------------
void FourTracksTab::setPatchPolicy(PatchPolicy policy)
//---------------------------------------------------
{
  patchPolicy_ = policy;

  for (auto* w : trackWidgets_)
    w->setPatchPolicy(policy);
}

//-------------------------------------------------------------------------
void FourTracksTab::setInstrumentDefinition(const InstrumentDefinition* def)
//-------------------------------------------------------------------------
{
  instrumentDefinition_ = def;

  for (auto* w : trackWidgets_)
    w->setInstrumentDefinition(def);
}

//---------------------------------------------------------------------------------------------------------
void FourTracksTab::onInstrumentProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program)
//---------------------------------------------------------------------------------------------------------
{
  emit instrumentProgramSelected(trackIdx, msb, lsb, program);
}

//-----------------------------------------------------------------------------------------------------
void FourTracksTab::onManualProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program)
//-----------------------------------------------------------------------------------------------------
{
  emit manualProgramSelected(trackIdx, msb, lsb, program);
}
