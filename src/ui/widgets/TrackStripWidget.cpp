#include "TrackStripWidget.h"
#include "../../../../Common/src/ui/widgets/MidiValueSelector.h"
#include "../../../../Common/src/ui/widgets/TouchComboBox.h"

#include "../../core/Presets.h"
#include "../../core/GroupUtils.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QSizePolicy>

#include <set>

//---------------------------------------------------------------------------------------
TrackStripWidget::TrackStripWidget(QWidget* parent, uint8_t trackIndex) : QWidget(parent)
//---------------------------------------------------------------------------------------
{
  trackIndex_ = trackIndex;
  buildUi(trackIndex);
  updateVisibility();
}

//-----------------------------------------------------------
void TrackStripWidget::setPlacement(GroupPlacement placement)
//-----------------------------------------------------------
{
  placement_ = placement;
  titleLabel_->setText(tr("Track %1").arg(int(trackIndex_) + 1));
  updateVisibility();
}

//------------------------------------------------
void TrackStripWidget::buildUi(uint8_t trackIndex)
//------------------------------------------------
{
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(6, 6, 6, 6);
  root->setSpacing(4);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

  titleLabel_ = new QLabel(tr("Track %1").arg(int(trackIndex) + 1));
  titleLabel_->setAlignment(Qt::AlignCenter);
  titleLabel_->setFixedHeight(titleLabel_->sizeHint().height());
  titleLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  root->addWidget(titleLabel_);

  // ----------------------------
  // Combo group
  // ----------------------------
  groupCombo_ = new TouchComboBox(this);
  groupCombo_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  auto addGroupItem = [this](TrackGroupId group)
    {
      groupCombo_->addItem(groupName(group), static_cast<int>(group));
    };

  addGroupItem(TrackGroupId::None);
  addGroupItem(TrackGroupId::Forte);
  addGroupItem(TrackGroupId::TrebleForte);
  addGroupItem(TrackGroupId::Treble);
  addGroupItem(TrackGroupId::TreblePiano);
  addGroupItem(TrackGroupId::Piano);
  addGroupItem(TrackGroupId::BassPiano);
  addGroupItem(TrackGroupId::Bass);
  addGroupItem(TrackGroupId::BassForte);

  groupCombo_->setCurrentIndex(groupCombo_->findData(static_cast<int>(TrackGroupId::None)));

  connect(groupCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &TrackStripWidget::onGroupChanged);

  root->addWidget(groupCombo_);

  activeControlsWidget_ = new QWidget(this);
  activeControlsWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  auto* controlsLayout = new QVBoxLayout(activeControlsWidget_);
  controlsLayout->setContentsMargins(0, 0, 0, 0);
  controlsLayout->setSpacing(4);
  root->addWidget(activeControlsWidget_);

  // ----------------------------
  // Instrument page
  // ----------------------------
  patchStack_ = new QStackedWidget(activeControlsWidget_);

  {
    instrumentPage_ = new QWidget(patchStack_);

    auto* instrumentLayout = new QGridLayout(instrumentPage_);
    instrumentLayout->setContentsMargins(0, 0, 0, 0);
    instrumentLayout->setHorizontalSpacing(4);
    instrumentLayout->setVerticalSpacing(2);

    auto* categoryLabel = new QLabel("Category", instrumentPage_);
    auto* programLabel = new QLabel("Program", instrumentPage_);

    categoryCombo_ = new TouchComboBox(instrumentPage_);
    patchCombo_ = new TouchComboBox(instrumentPage_);

    instrumentLayout->addWidget(categoryLabel, 0, 0);
    instrumentLayout->addWidget(programLabel, 0, 1);

    instrumentLayout->addWidget(categoryCombo_, 1, 0);
    instrumentLayout->addWidget(patchCombo_, 1, 1);

    instrumentLayout->setColumnStretch(0, 1);
    instrumentLayout->setColumnStretch(1, 2);

    categoryLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    programLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
  }
  patchStack_->addWidget(instrumentPage_);

  connect(categoryCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &TrackStripWidget::onInstrumentCategoryChanged);
  connect(patchCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &TrackStripWidget::onInstrumentPatchChanged);

  // ----------------------------
  // Manual page
  // ----------------------------
  {
    manualPage_ = new QWidget(patchStack_);

    auto* manualLayout = new QGridLayout(manualPage_);
    manualLayout->setContentsMargins(0, 0, 0, 0);
    manualLayout->setHorizontalSpacing(4);
    manualLayout->setVerticalSpacing(2);

    auto* msbLabel = new QLabel("Bank MSB", manualPage_);
    auto* lsbLabel = new QLabel("Bank LSB", manualPage_);
    auto* programLabel = new QLabel("Program", manualPage_);

    msbSpin_ = new MidiValueSelector(manualPage_);
    msbSpin_->setRange(0, 127);
    msbSpin_->setValue(0);

    lsbSpin_ = new MidiValueSelector(manualPage_);
    lsbSpin_->setRange(0, 127);
    lsbSpin_->setValue(0);

    gmProgramCombo_ = new TouchComboBox(manualPage_);

    manualLayout->addWidget(msbLabel, 0, 0);
    manualLayout->addWidget(lsbLabel, 0, 1);
    manualLayout->addWidget(programLabel, 0, 2);

    manualLayout->addWidget(msbSpin_, 1, 0);
    manualLayout->addWidget(lsbSpin_, 1, 1);
    manualLayout->addWidget(gmProgramCombo_, 1, 2);

    manualLayout->setColumnStretch(0, 0);
    manualLayout->setColumnStretch(1, 0);
    manualLayout->setColumnStretch(2, 1);

    msbLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    lsbLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    programLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
  }
  patchStack_->addWidget(manualPage_);

  connect(msbSpin_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onManualMsbChanged);
  connect(lsbSpin_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onManualLsbChanged);
  connect(gmProgramCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &TrackStripWidget::onManualProgramChanged);

  rebuildManualProgramCombo();
  setPatchPolicy(patchPolicy_);

  controlsLayout->addWidget(patchStack_);
  //root->addLayout(fullProgramLayout);
  controlsLayout->addSpacing(10);

  // ----------------------------
  // combo Footage
  // ----------------------------
  {
    footageCombo_ = new TouchComboBox(activeControlsWidget_);
    footageCombo_->addItems({
        "16'",    // + 0 cents
        "8'",     // + 0 cents
        "5 1/3'", // + 2 cents
        "4'",     // + 0 cents
        "2 2/3'", // + 2 cents
        "2'",     // + 0 cents
        "1 3/5'", // -14 cents
        "1 1/3'", // + 2 cents
        "1 1/7'", // -31 cents
        "1'"      // + 0 cents
      });
    footageCombo_->setCurrentText("8'");

    controlsLayout->addWidget(new QLabel(tr("Footage"), activeControlsWidget_));
    controlsLayout->addWidget(footageCombo_);

    connect(footageCombo_, &QComboBox::currentIndexChanged, this, &TrackStripWidget::onFootageChanged);
  }

  // -----------------------------
  // volume, pan, timbre1, timbre2
  // -----------------------------
  {
    auto* volumeRow = new QVBoxLayout();
    volumeRow->setSpacing(2);

    volumeSlider_ = new MidiValueSelector(activeControlsWidget_);
    volumeSlider_->setRange(0, 127);

    connect(volumeSlider_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onVolumeChanged);

    volumeLabel_ = new QLabel(tr("Volume"), activeControlsWidget_);
    volumeRow->addWidget(volumeLabel_, 0, Qt::AlignHCenter);
    volumeRow->addWidget(volumeSlider_, 0, Qt::AlignHCenter);

    controlsLayout->addLayout(volumeRow);
    controlsLayout->addSpacing(10);
  }

  // Pan
  {
    panKnob_ = new MidiValueSelector(activeControlsWidget_);
    panKnob_->setRange(-64, 63);

    connect(panKnob_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onPanChanged);

    auto* panRow = new QVBoxLayout();
    panRow->setSpacing(2);
    panLabel_ = new QLabel(tr("Pan"), activeControlsWidget_);
    panRow->addWidget(panLabel_, 0, Qt::AlignHCenter);
    panRow->addWidget(panKnob_, 0, Qt::AlignHCenter);

    controlsLayout->addLayout(panRow);
  }

  // Reverb
  {
    reverbKnob_ = new MidiValueSelector(activeControlsWidget_);
    reverbKnob_->setRange(0, 127);

    connect(reverbKnob_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onReverbChanged);

    auto* reverbRow = new QVBoxLayout();
    reverbRow->setSpacing(2);
    reverbLabel_ = new QLabel(tr("Reverb"), activeControlsWidget_);
    reverbRow->addWidget(reverbLabel_, 0, Qt::AlignHCenter);
    reverbRow->addWidget(reverbKnob_, 0, Qt::AlignHCenter);

    controlsLayout->addLayout(reverbRow);
  }

  // Chorus
  {
    chorusKnob_ = new MidiValueSelector(activeControlsWidget_);
    chorusKnob_->setRange(0, 127);

    connect(chorusKnob_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onChorusChanged);

    auto* chorusRow = new QVBoxLayout();
    chorusRow->setSpacing(2);
    chorusLabel_ = new QLabel(tr("Chorus"), activeControlsWidget_);
    chorusRow->addWidget(chorusLabel_, 0, Qt::AlignHCenter);
    chorusRow->addWidget(chorusKnob_, 0, Qt::AlignHCenter);

    controlsLayout->addLayout(chorusRow);
  }

  // Timbre1
  {
    timbre1Knob_ = new MidiValueSelector(activeControlsWidget_);
    timbre1Knob_->setRange(-64, 63);

    connect(timbre1Knob_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onTimbre1Changed);

    auto* timbre1Row = new QVBoxLayout();
    timbre1Row->setSpacing(2);
    timbre1Label_ = new QLabel(tr("Tone"), activeControlsWidget_);
    timbre1Row->addWidget(timbre1Label_, 0, Qt::AlignHCenter);
    timbre1Row->addWidget(timbre1Knob_, 0, Qt::AlignHCenter);

    controlsLayout->addLayout(timbre1Row);
  }

  // Timbre2
  {
    timbre2Knob_ = new MidiValueSelector(activeControlsWidget_);
    timbre2Knob_->setRange(-64, 63);

    connect(timbre2Knob_, &MidiValueSelector::valueChanged, this, &TrackStripWidget::onTimbre2Changed);

    auto* timbre2Row = new QVBoxLayout();
    timbre2Row->setSpacing(2);
    timbre2Label_ = new QLabel(tr("Color"), activeControlsWidget_);
    timbre2Row->addWidget(timbre2Label_, 0, Qt::AlignHCenter);
    timbre2Row->addWidget(timbre2Knob_, 0, Qt::AlignHCenter);

    controlsLayout->addLayout(timbre2Row);
  }

  // Default values

  panKnob_->setValue(-1);

  reverbKnob_->setValue(1);
  chorusKnob_->setValue(1);

  timbre1Knob_->setValue(-1);
  timbre2Knob_->setValue(-1);

  volumeSlider_->setValue(100);

  panKnob_->setValue(0);

  reverbKnob_->setValue(40);
  chorusKnob_->setValue(0);

  timbre1Knob_->setValue(0);
  timbre2Knob_->setValue(0);

  // -----------------------------
  // Curves group box
  // -----------------------------
  auto* curvesGroup = new QGroupBox(tr("Curves"), activeControlsWidget_);
  auto* curvesLayout = new QVBoxLayout(curvesGroup);
  curvesLayout->setContentsMargins(6, 6, 6, 6);
  curvesLayout->setSpacing(4);

  keyExprLabel_ = new QLabel(tr("Note morph curve"), activeControlsWidget_);
  keyExprCombo_ = new TouchComboBox(activeControlsWidget_);
  keyExprCombo_->addItems({ "Curve N1", "Curve N2", "Curve N3", "Curve N4" });

  velExprLabel_ = new QLabel(tr("Velocity morph curve"), activeControlsWidget_);
  velExprCombo_ = new TouchComboBox(activeControlsWidget_);
  velExprCombo_->addItems({ "Curve V1", "Curve V2", "Curve V3", "Curve V4" });

  curvesLayout->addWidget(keyExprLabel_);
  curvesLayout->addWidget(keyExprCombo_);

  curvesLayout->addWidget(velExprLabel_);
  curvesLayout->addWidget(velExprCombo_);

  controlsLayout->addWidget(curvesGroup);

  setMinimumWidth(160);
}

//---------------------------------------
void TrackStripWidget::updateVisibility()
//---------------------------------------
{
  const bool active = isActive();

  if (activeControlsWidget_)
    activeControlsWidget_->setVisible(active);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  updateGeometry();

  if (!active)
    return;

  const bool showKey = (placement_ != GroupPlacement::VelocityOnly);
  const bool showVel = (placement_ != GroupPlacement::KeyOnly);

  if (keyExprLabel_)
    keyExprLabel_->setVisible(showKey);
  if (keyExprCombo_)
    keyExprCombo_->setVisible(showKey);

  if (velExprLabel_)
    velExprLabel_->setVisible(showVel);
  if (velExprCombo_)
    velExprCombo_->setVisible(showVel);
}

//---------------------------------------------
void TrackStripWidget::onGroupChanged(int index)
//---------------------------------------------
{
  Q_UNUSED(index);

  TrackGroupId group = groupId();
  const GroupMorphProfile gp = GroupMorphProfile::GetProfile(group);
  if (gp.useKey && gp.useVelocity)
    setPlacement(GroupPlacement::BothAxes);
  else if (gp.useKey)
    setPlacement(GroupPlacement::KeyOnly);
  else if (gp.useVelocity)
    setPlacement(GroupPlacement::VelocityOnly);
  else
    updateVisibility();

  //mandare i controlli correnti quando una traccia diventa attiva, in modo da avere sempre lo stato uguale in macchina e qua. Poi se uno fa editing sulla macchina fatti suoi.
  //Questo implica che sarebbe il caso di definire delle impostazioni di default per traccia, in modo da non partire sempre con tutti i pianoforti, zero riverbero, ecc...

  emit trackGroupChanged(trackIndex_, groupId());
  emit trackActiveChanged(trackIndex_, isActive());
}

//--------------------------------------------
TrackGroupId TrackStripWidget::groupId() const
//--------------------------------------------
{
  if (!groupCombo_)
    return TrackGroupId::None;

  const QVariant data = groupCombo_->currentData();
  if (!data.isValid())
    return TrackGroupId::None;

  return static_cast<TrackGroupId>(data.toInt());
}

//-----------------------------------------------------
void TrackStripWidget::setGroupId(TrackGroupId groupId)
//-----------------------------------------------------
{
  if (!groupCombo_)
    return;

  const int groupIdx = groupCombo_->findData(static_cast<int>(groupId));
  if (groupIdx >= 0)
  {
    QSignalBlocker b(groupCombo_);
    groupCombo_->setCurrentIndex(groupIdx);
  }
  updateVisibility();
}

//-----------------------------------
bool TrackStripWidget::isActive() const
//-----------------------------------
{
  return groupId() != TrackGroupId::None;
}

//--------------------------------------------------------
ExpressionCurveId TrackStripWidget::keyExprCurveId() const
//--------------------------------------------------------
{
  return static_cast<ExpressionCurveId>(keyExprCombo_->currentIndex());
}

//--------------------------------------------------------
ExpressionCurveId TrackStripWidget::velExprCurveId() const
//--------------------------------------------------------
{
  return static_cast<ExpressionCurveId>(velExprCombo_->currentIndex());
}

//-------------------------------------------
int8_t TrackStripWidget::timbre1Value() const
//-------------------------------------------
{
  return (int8_t)timbre1Knob_->value();
}

//-------------------------------------------
int8_t TrackStripWidget::timbre2Value() const
//-------------------------------------------
{
  return (int8_t)timbre2Knob_->value();
}

//--------------------------------------------
void TrackStripWidget::resetTrackMidiWidgets()
//--------------------------------------------
{
  QSignalBlocker b1(cc00Spin_);
  QSignalBlocker b2(cc32Spin_);
  QSignalBlocker b3(comboProgram_);
  QSignalBlocker b4(footageCombo_);
  QSignalBlocker b5(volumeSlider_);
  QSignalBlocker b6(panKnob_);
  QSignalBlocker b7(timbre1Knob_);
  QSignalBlocker b8(timbre2Knob_);

  if (cc00Spin_)
    cc00Spin_->setValue(0);

  if (cc32Spin_)
    cc32Spin_->setValue(0);

  if (comboProgram_)
    comboProgram_->setCurrentIndex(0);

  if (footageCombo_)
    footageCombo_->setCurrentIndex((int)Footage::ftg8);

  if (volumeSlider_)
    volumeSlider_->setValue(100);

  if (panKnob_)
    panKnob_->setValue(0);

  if (timbre1Knob_)
    timbre1Knob_->setValue(0);

  if (timbre2Knob_)
    timbre2Knob_->setValue(0);
}

//----------------------------------------------------------------------
void TrackStripWidget::setTrackPresetData(const TrackPresetData& preset)
//----------------------------------------------------------------------
{
  if (groupCombo_)
    setGroupId(preset.group);

  if (cc00Spin_)
  {
    QSignalBlocker b1(cc00Spin_);
    cc00Spin_->setValue(preset.program_cc0);
  }
  if (cc32Spin_)
  {
    QSignalBlocker b2(cc32Spin_);
    cc32Spin_->setValue(preset.program_cc32);
  }

  if (comboProgram_)
    comboProgram_->setCurrentIndex(preset.program_number);

  if (footageCombo_)
    footageCombo_->setCurrentIndex((int)preset.footage);
  if (volumeSlider_)
    volumeSlider_->setValue(preset.volume);
  if (panKnob_)
    panKnob_->setValue(preset.panorama);

  if (reverbKnob_)
    reverbKnob_->setValue(preset.reverb);
  if (chorusKnob_)
    chorusKnob_->setValue(preset.chorus);

  if (timbre1Knob_)
    timbre1Knob_->setValue(preset.timbre1);
  if (timbre2Knob_)
    timbre2Knob_->setValue(preset.timbre2);

  if (keyExprCombo_)
    keyExprCombo_->setCurrentIndex(preset.curve1);
  if (velExprCombo_)
    velExprCombo_->setCurrentIndex(preset.curve2);
}

//----------------------------------------------------------------------
void TrackStripWidget::getTrackPresetData(TrackPresetData& preset) const
//----------------------------------------------------------------------
{
  if (groupCombo_)
  {
    preset.group = static_cast<TrackGroupId>(groupCombo_->currentData().toInt());
    preset.valid = (preset.group != TrackGroupId::None);
  }

  if (cc00Spin_)
    preset.program_cc0 = cc00Spin_->value();
  if (cc32Spin_)
    preset.program_cc32 = cc32Spin_->value();
  if (comboProgram_)
    preset.program_number = comboProgram_->currentIndex();

  if (footageCombo_)
    preset.footage = static_cast<Footage>(footageCombo_->currentIndex());
  if (volumeSlider_)
    preset.volume = volumeSlider_->value();
  if (panKnob_)
    preset.panorama = panKnob_->value();

  if (reverbKnob_)
    preset.reverb = reverbKnob_->value();
  if (chorusKnob_)
    preset.chorus = chorusKnob_->value();

  if (timbre1Knob_)
    preset.timbre1 = timbre1Knob_->value();
  if (timbre2Knob_)
    preset.timbre2 = timbre2Knob_->value();

  if (keyExprCombo_)
    preset.curve1 = keyExprCombo_->currentIndex();
  if (velExprCombo_)
    preset.curve2 = velExprCombo_->currentIndex();
}

//-------------------------------------------------------
void TrackStripWidget::setPatchPolicy(PatchPolicy policy)
//-------------------------------------------------------
{
  patchPolicy_ = policy;

  if (!patchStack_)
    return;

  if (patchPolicy_ == PatchPolicy::Instrument)
    patchStack_->setCurrentWidget(instrumentPage_);
  else
    patchStack_->setCurrentWidget(manualPage_);
}

//-----------------------------------------------------------------------------
void TrackStripWidget::setInstrumentDefinition(const InstrumentDefinition* def)
//-----------------------------------------------------------------------------
{
  instrumentDefinition_ = def;
  rebuildInstrumentCategoryCombo();
}

//--------------------------------------------------
void TrackStripWidget::onFootageChanged(uint8_t idx)
//--------------------------------------------------
{
  Footage f = static_cast<Footage>(idx);
  emit trackFootageChanged(trackIndex_, f);
}

//------------------------------------------
void TrackStripWidget::onProgramChanged(int)
//------------------------------------------
{
  uint8_t cc00 = cc00Spin_->value();
  uint8_t cc32 = cc32Spin_->value();
  uint8_t prog = comboProgram_->currentIndex();
  emit trackProgramChanged(trackIndex_, cc00, cc32, prog);
}

//-----------------------------------------------
void TrackStripWidget::onVolumeChanged(int value)
//-----------------------------------------------
{
  emit trackVolumeChanged(trackIndex_, value);
}

//--------------------------------------------
void TrackStripWidget::onPanChanged(int value)
//--------------------------------------------
{
  emit trackPanChanged(trackIndex_, value + 64);
}

//-----------------------------------------------
void TrackStripWidget::onReverbChanged(int value)
//-----------------------------------------------
{
  emit trackReverbChanged(trackIndex_, value);
}

//-----------------------------------------------
void TrackStripWidget::onChorusChanged(int value)
//-----------------------------------------------
{
  emit trackChorusChanged(trackIndex_, value);
}

//------------------------------------------------
void TrackStripWidget::onTimbre1Changed(int value)
//------------------------------------------------
{
  emit trackTimbre1Changed(trackIndex_, value);
}

//------------------------------------------------
void TrackStripWidget::onTimbre2Changed(int value)
//------------------------------------------------
{
  emit trackTimbre2Changed(trackIndex_, value);
}

//-----------------------------------
void TrackStripWidget::buildPatchUi()
//-----------------------------------
{
  patchStack_ = new QStackedWidget(this);

  // ----------------------------
  // Instrument page
  // ----------------------------
  instrumentPage_ = new QWidget(patchStack_);
  auto* instrumentLayout = new QHBoxLayout(instrumentPage_);
  instrumentLayout->setContentsMargins(0, 0, 0, 0);

  categoryCombo_ = new TouchComboBox(instrumentPage_);
  patchCombo_ = new TouchComboBox(instrumentPage_);

  instrumentLayout->addWidget(categoryCombo_, 1);
  instrumentLayout->addWidget(patchCombo_, 2);

  connect(categoryCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &TrackStripWidget::onInstrumentCategoryChanged);
  connect(patchCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &TrackStripWidget::onInstrumentPatchChanged);

  patchStack_->addWidget(instrumentPage_);

  // ----------------------------
  // Manual page
  // ----------------------------
  manualPage_ = new QWidget(patchStack_);
  auto* manualLayout = new QHBoxLayout(manualPage_);
  manualLayout->setContentsMargins(0, 0, 0, 0);

  msbSpin_ = new MidiValueSelector(manualPage_);
  msbSpin_->setRange(0, 127);
  msbSpin_->setValue(0);

  lsbSpin_ = new MidiValueSelector(manualPage_);
  lsbSpin_->setRange(0, 127);
  lsbSpin_->setValue(0);

  gmProgramCombo_ = new TouchComboBox(manualPage_);

  manualLayout->addWidget(msbSpin_);
  manualLayout->addWidget(lsbSpin_);
  manualLayout->addWidget(gmProgramCombo_, 1);

  connect(msbSpin_, &MidiValueSelector::valueChanged,
    this, &TrackStripWidget::onManualMsbChanged);

  connect(lsbSpin_, &MidiValueSelector::valueChanged,
    this, &TrackStripWidget::onManualLsbChanged);

  connect(gmProgramCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
    this, &TrackStripWidget::onManualProgramChanged);

  patchStack_->addWidget(manualPage_);

  rebuildManualProgramCombo();
  setPatchPolicy(patchPolicy_);
}

//-----------------------------------------------------
void TrackStripWidget::rebuildInstrumentCategoryCombo()
//-----------------------------------------------------
{
  if (!categoryCombo_)
    return;

  QSignalBlocker blocker(categoryCombo_);
  categoryCombo_->clear();

  if (!instrumentDefinition_ || instrumentDefinition_->programs.empty())
  {
    rebuildInstrumentPatchCombo();
    return;
  }

  std::set<ProgramCategory> categories;
  for (const auto& p : instrumentDefinition_->programs)
    categories.insert(p.category);

  for (ProgramCategory c : categories)
    categoryCombo_->addItem(QString::fromStdString(category_name[static_cast<uint8_t>(c)]), static_cast<int>(c));

  if (categoryCombo_->count() > 0)
    categoryCombo_->setCurrentIndex(0);

  rebuildInstrumentPatchCombo();
}

//--------------------------------------------------
void TrackStripWidget::rebuildInstrumentPatchCombo()
//--------------------------------------------------
{
  if (!patchCombo_)
    return;

  QSignalBlocker blocker(patchCombo_);
  patchCombo_->clear();
  filteredPrograms_.clear();

  if (!instrumentDefinition_ || !categoryCombo_ || categoryCombo_->currentIndex() < 0)
    return;

  const auto category = static_cast<ProgramCategory>(categoryCombo_->currentData().toInt());

  for (const auto& p : instrumentDefinition_->programs)
  {
    if (p.category != category)
      continue;

    filteredPrograms_.push_back(&p);

    const QString text = QString::fromStdString(p.name) + QString(" (%1, %2, %3)").arg(p.msb).arg(p.lsb).arg(p.program + 1);

    patchCombo_->addItem(text);
  }

  if (patchCombo_->count() > 0)
    patchCombo_->setCurrentIndex(0);
}

//-----------------------------------------------------------
void TrackStripWidget::onInstrumentCategoryChanged(int index)
//-----------------------------------------------------------
{
  Q_UNUSED(index);
  rebuildInstrumentPatchCombo();
  gmProgramCombo_->setCurrentIndex(-1);
}

//--------------------------------------------------------
void TrackStripWidget::onInstrumentPatchChanged(int index)
//--------------------------------------------------------
{
  if (index < 0 || index >= static_cast<int>(filteredPrograms_.size()))
    return;

  const ProgramEntry* p = filteredPrograms_[static_cast<size_t>(index)];
  if (!p)
    return;

  emit instrumentProgramSelected(trackIndex_, p->msb, p->lsb, p->program);
}

//---------------------------------------------------------
QString TrackStripWidget::gmProgramDisplayName(int program)
//---------------------------------------------------------
{
  static const char* kNames[128] =
  {
      "001 Acoustic Grand Piano",
      "002 Bright Acoustic Piano",
      "003 Electric Grand Piano",
      "004 Honky-tonk Piano",
      "005 Electric Piano 1",
      "006 Electric Piano 2",
      "007 Harpsichord",
      "008 Clavi",
      "009 Celesta",
      "010 Glockenspiel",
      "011 Music Box",
      "012 Vibraphone",
      "013 Marimba",
      "014 Xylophone",
      "015 Tubular Bells",
      "016 Dulcimer",
      "017 Drawbar Organ",
      "018 Percussive Organ",
      "019 Rock Organ",
      "020 Church Organ",
      "021 Reed Organ",
      "022 Accordion",
      "023 Harmonica",
      "024 Tango Accordion",
      "025 Acoustic Guitar (nylon)",
      "026 Acoustic Guitar (steel)",
      "027 Electric Guitar (jazz)",
      "028 Electric Guitar (clean)",
      "029 Electric Guitar (muted)",
      "030 Overdriven Guitar",
      "031 Distortion Guitar",
      "032 Guitar Harmonics",
      "033 Acoustic Bass",
      "034 Electric Bass (finger)",
      "035 Electric Bass (pick)",
      "036 Fretless Bass",
      "037 Slap Bass 1",
      "038 Slap Bass 2",
      "039 Synth Bass 1",
      "040 Synth Bass 2",
      "041 Violin",
      "042 Viola",
      "043 Cello",
      "044 Contrabass",
      "045 Tremolo Strings",
      "046 Pizzicato Strings",
      "047 Orchestral Harp",
      "048 Timpani",
      "049 String Ensemble 1",
      "050 String Ensemble 2",
      "051 Synth Strings 1",
      "052 Synth Strings 2",
      "053 Choir Aahs",
      "054 Voice Oohs",
      "055 Synth Voice",
      "056 Orchestra Hit",
      "057 Trumpet",
      "058 Trombone",
      "059 Tuba",
      "060 Muted Trumpet",
      "061 French Horn",
      "062 Brass Section",
      "063 Synth Brass 1",
      "064 Synth Brass 2",
      "065 Soprano Sax",
      "066 Alto Sax",
      "067 Tenor Sax",
      "068 Baritone Sax",
      "069 Oboe",
      "070 English Horn",
      "071 Bassoon",
      "072 Clarinet",
      "073 Piccolo",
      "074 Flute",
      "075 Recorder",
      "076 Pan Flute",
      "077 Blown Bottle",
      "078 Shakuhachi",
      "079 Whistle",
      "080 Ocarina",
      "081 Lead 1 (square)",
      "082 Lead 2 (sawtooth)",
      "083 Lead 3 (calliope)",
      "084 Lead 4 (chiff)",
      "085 Lead 5 (charang)",
      "086 Lead 6 (voice)",
      "087 Lead 7 (fifths)",
      "088 Lead 8 (bass + lead)",
      "089 Pad 1 (new age)",
      "090 Pad 2 (warm)",
      "091 Pad 3 (polysynth)",
      "092 Pad 4 (choir)",
      "093 Pad 5 (bowed)",
      "094 Pad 6 (metallic)",
      "095 Pad 7 (halo)",
      "096 Pad 8 (sweep)",
      "097 FX 1 (rain)",
      "098 FX 2 (soundtrack)",
      "099 FX 3 (crystal)",
      "100 FX 4 (atmosphere)",
      "101 FX 5 (brightness)",
      "102 FX 6 (goblins)",
      "103 FX 7 (echoes)",
      "104 FX 8 (sci-fi)",
      "105 Sitar",
      "106 Banjo",
      "107 Shamisen",
      "108 Koto",
      "109 Kalimba",
      "110 Bag Pipe",
      "111 Fiddle",
      "112 Shanai",
      "113 Tinkle Bell",
      "114 Agogo",
      "115 Steel Drums",
      "116 Woodblock",
      "117 Taiko Drum",
      "118 Melodic Tom",
      "119 Synth Drum",
      "120 Reverse Cymbal",
      "121 Guitar Fret Noise",
      "122 Breath Noise",
      "123 Seashore",
      "124 Bird Tweet",
      "125 Telephone Ring",
      "126 Helicopter",
      "127 Applause",
      "128 Gunshot"
  };

  if (program < 0 || program >= 128)
    return QString();

  return QString::fromLatin1(kNames[program]);
}

//------------------------------------------------
void TrackStripWidget::rebuildManualProgramCombo()
//------------------------------------------------
{
  if (!gmProgramCombo_)
    return;

  QSignalBlocker blocker(gmProgramCombo_);
  gmProgramCombo_->clear();

  for (int i = 0; i < 128; ++i)
    gmProgramCombo_->addItem(gmProgramDisplayName(i), i);

  gmProgramCombo_->setCurrentIndex(-1);
}

//--------------------------------------------------
void TrackStripWidget::onManualMsbChanged(int value)
//--------------------------------------------------
{
  Q_UNUSED(value);
}

//--------------------------------------------------
void TrackStripWidget::onManualLsbChanged(int value)
//--------------------------------------------------
{
  Q_UNUSED(value);
}

//------------------------------------------------------
void TrackStripWidget::onManualProgramChanged(int index)
//------------------------------------------------------
{
  if (index < 0)
    return;

  const int msb = msbSpin_->value();
  const int lsb = lsbSpin_->value();
  const int program = gmProgramCombo_->currentData().toInt();

  emit manualProgramSelected(trackIndex_, msb, lsb, program);
}