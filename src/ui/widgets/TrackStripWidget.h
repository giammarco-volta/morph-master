#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <cstdint>

#include "../../core/ExpressionCurveId.h"
#include "../../core/Footage.h"
#include "../../core/InstrumentDefinition.h"
#include "../../core/TrackGroupId.h"
#include "../../core/GroupPlacement.h"

class QLabel;
class QComboBox;
class MidiValueSelector;
struct TrackPresetData;

class TrackStripWidget : public QWidget
{
  Q_OBJECT

public:
  explicit TrackStripWidget(QWidget* parent, uint8_t trackIndex);

  void setPlacement(GroupPlacement placement);

  uint8_t trackIndex() const { return trackIndex_; }
  TrackGroupId groupId() const;
  void setGroupId(TrackGroupId groupId);
  bool isActive() const;
  ExpressionCurveId keyExprCurveId() const;
  ExpressionCurveId velExprCurveId() const;
  int8_t timbre1Value() const;
  int8_t timbre2Value() const;

  void resetTrackMidiWidgets();
  void setTrackPresetData(const TrackPresetData& preset);
  void getTrackPresetData(TrackPresetData& preset) const;

  void setPatchPolicy(PatchPolicy policy);
  void setInstrumentDefinition(const InstrumentDefinition* def);

  int currentManualMsb() const;
  int currentManualLsb() const;
  int currentManualProgram() const;

  const ProgramEntry* currentInstrumentProgram() const;

private:
  void buildUi(uint8_t trackIndex);
  void updateVisibility();
  void onGroupChanged(int index);

  void onFootageChanged(uint8_t idx);
  void onProgramChanged(int);
  void onVolumeChanged(int value);
  void onPanChanged(int);
  void onReverbChanged(int);
  void onChorusChanged(int);
  void onTimbre1Changed(int);
  void onTimbre2Changed(int);

  void buildPatchUi();
  void rebuildInstrumentCategoryCombo();
  void rebuildInstrumentPatchCombo();
  void rebuildManualProgramCombo();

  void onInstrumentCategoryChanged(int index);
  void onInstrumentPatchChanged(int index);
  void onManualMsbChanged(int value);
  void onManualLsbChanged(int value);
  void onManualProgramChanged(int index);

  static QString gmProgramDisplayName(int program);

signals:
  void trackFootageChanged(uint8_t trackIdx, Footage f);
  void trackProgramChanged(uint8_t trackIdx, uint8_t cc00, uint8_t cc32, uint8_t program);
  void trackVolumeChanged(uint8_t trackIdx, uint8_t value);
  void trackPanChanged(uint8_t trackIdx, uint8_t value);

  void trackReverbChanged(uint8_t trackIdx, uint8_t value);
  void trackChorusChanged(uint8_t trackIdx, uint8_t value);
  void trackTimbre1Changed(uint8_t trackIdx, uint8_t value);
  void trackTimbre2Changed(uint8_t trackIdx, uint8_t value);

  void instrumentProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program);
  void manualProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program);

  void trackGroupChanged(uint8_t trackIdx, TrackGroupId group);
  void trackActiveChanged(uint8_t trackIdx, bool active);

private:
  GroupPlacement placement_ = GroupPlacement::BothAxes;

  uint8_t trackIndex_ = 0;

  QComboBox* groupCombo_ = nullptr;

  QLabel* titleLabel_ = nullptr;
  QWidget* activeControlsWidget_ = nullptr;

  QComboBox* footageCombo_ = nullptr;

  QComboBox* comboProgram_ = nullptr;
  MidiValueSelector* cc00Spin_ = nullptr;
  MidiValueSelector* cc32Spin_ = nullptr;

  QLabel* volumeLabel_ = nullptr;
  MidiValueSelector* volumeSlider_ = nullptr;

  QLabel* panLabel_ = nullptr;
  MidiValueSelector* panKnob_ = nullptr;

  QLabel* timbre1Label_ = nullptr;
  MidiValueSelector* timbre1Knob_ = nullptr;

  QLabel* timbre2Label_ = nullptr;
  MidiValueSelector* timbre2Knob_ = nullptr;

  QLabel* reverbLabel_ = nullptr;
  MidiValueSelector* reverbKnob_ = nullptr;

  QLabel* chorusLabel_ = nullptr;
  MidiValueSelector* chorusKnob_ = nullptr;

  QLabel* keyExprLabel_ = nullptr;
  QComboBox* keyExprCombo_ = nullptr;

  QLabel* velExprLabel_ = nullptr;
  QComboBox* velExprCombo_ = nullptr;

  PatchPolicy patchPolicy_ = PatchPolicy::Manual;
  const InstrumentDefinition* instrumentDefinition_ = nullptr;

  QStackedWidget* patchStack_ = nullptr;

  QWidget* instrumentPage_ = nullptr;
  QComboBox* categoryCombo_ = nullptr;
  QComboBox* patchCombo_ = nullptr;

  QWidget* manualPage_ = nullptr;
  MidiValueSelector* msbSpin_ = nullptr;
  MidiValueSelector* lsbSpin_ = nullptr;
  QComboBox* gmProgramCombo_ = nullptr;

  std::vector<const ProgramEntry*> filteredPrograms_;
};