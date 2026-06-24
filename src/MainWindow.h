#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QFutureWatcher>
#include <memory>

#include <bitset>
#include <optional>

#include "core/Presets.h"
#include "core/InstrumentDefinition.h"
#include "core/InstrumentDatabase.h"

class QTabWidget;
class MidiSettingsTab;
class SurfaceTab;
class CurveEditorTab;
class FourTracksTab;
class IMidiOut;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

private:
  void handleIncomingNote(uint8_t note, uint8_t velocity, IMidiOut* out);
  void handleIncomingNoteOff(uint8_t note, uint8_t velocity, IMidiOut* out);
  void handleIncomingChannelMsg(uint8_t code, uint8_t data1, uint8_t data2, IMidiOut* out);

  PresetData captureCurrentPreset(const QString& name) const;
  void applyPreset(const PresetData& preset);
  void gatherPresetData(PresetData& preset);

  void refreshPresetUi();

  //void loadInstrumentDatabase();
  void startInstrumentDatabaseLoading();

  AppInitSettings loadInitSettings() const;
  void saveInitSettings() const;

  std::list<uint8_t> getTracksForGroup(TrackGroupId groupId) const;

  static QString playModeToString(PlayMode mode);
  static QString patchPolicyToString(PatchPolicy mode);
  static PlayMode playModeFromString(const QString& s);
  static PatchPolicy patchPolicyFromString(const QString& s);

  //void sendAllNotesOff();

public slots:
  void onTrackFootageChanged(uint8_t trackIdx, Footage f);
  void onTrackProgramChanged(uint8_t trackIdx, uint8_t cc00, uint8_t cc32, uint8_t program);
  void onTrackVolumeChanged(uint8_t trackIdx, uint8_t value);
  void onTrackPanChanged(uint8_t trackIdx, uint8_t value);

  void onTrackReverbChanged(uint8_t trackIdx, uint8_t value);
  void onTrackChorusChanged(uint8_t trackIdx, uint8_t value);
  void onTrackTimbre1Changed(uint8_t trackIdx, uint8_t value);
  void onTrackTimbre2Changed(uint8_t trackIdx, uint8_t value);

  void onButtonSave();
  void onButtonSaveAs();
  void onButtonDelete();
  void onPresetSelectionChanged(int);
  void onPlayModeChanged(int);

  void onInstrumentProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program);//from GroupTab
  void onManualProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program);//from GroupTab

  void onOutPortChanged(uint8_t idx);
  void onInPortChanged(uint8_t idx);
  void onInChannelChanged(uint8_t id);

private slots:
  void onMidiNoteOnReceived(uint8_t note, uint8_t velocity, IMidiOut* out);
  void onMidiNoteOffReceived(uint8_t note, uint8_t velocity, IMidiOut* out);
  void onMidiChannelMsgReceived(uint8_t code, uint8_t data1, uint8_t data2, IMidiOut* out);
  void onInstrumentDefinitionModeChanged(bool instrumentMode);
  void onKnownInstrumentChanged(const QString& instrumentName);
  void onTrackGroupChanged(uint8_t trackIdx, TrackGroupId groupId);
  void onTrackActivationChanged(uint8_t trackIdx, bool active);
  void onGroupBadgeTrackMaskEdited(TrackGroupId groupIndex, uint16_t trackMask);
  void onSurfaceTestNoteOn(uint8_t note, uint8_t velocity);
  void onSurfaceTestNoteOff(uint8_t note);
  void onKeyboardRangeChanged(uint8_t index);

private:
  MidiSettingsTab* midiSettingTab_ = nullptr;
  std::array<CurveEditorTab*, 2> curveEditorTab_{};
  SurfaceTab* surfaceTab_ = nullptr;
  std::array<FourTracksTab*, 4> fourTracksTabs_{};
  int8_t footageTransposition[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  bool applyingPreset_ = false;
  PresetManager* presetManager_ = nullptr;

  int8_t cc71contribute_[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int8_t cc74contribute_[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  PatchPolicy patchPolicy_ = PatchPolicy::Manual;
  InstrumentDefinition instrumentDefinition_;

  std::shared_ptr<InstrumentDatabase> instrumentDatabase_;

  PlayMode playMode_ = PlayMode::MonoRetrigVelOff;

  bool loadingPreset_ = false;

  struct HeldNote
  {
    uint8_t note = 0;
    uint8_t velocity = 0;
  };

  std::list<HeldNote> currNotes_;
  std::optional<HeldNote> monoPlayingNote_;

  std::bitset<16> activeTracks_;

  QFutureWatcher<std::shared_ptr<InstrumentDatabase>>* instrumentDbWatcher_ = nullptr;
  std::shared_ptr<InstrumentDatabase> pendingInstrumentDb_;
  QString pendingInstrumentName_;
};