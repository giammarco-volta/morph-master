#pragma once

#include <array>
#include <QObject>
#include <QStringList>
#include <QFutureWatcher>
#include <QString>
#include <QVariantList>

#include "../core/InstrumentDatabase.h"
#include "../core/Presets.h"

#include <memory>

class TrackController;
class CurveController;
class QThread;
class MidiEngine;
class MorphOutputStateModel;
class QAbstractItemModel;
struct MidiEngineSnapshot;

//---------------------------------
struct InstrumentDatabaseLoadResult
//---------------------------------
{
  bool ok = false;
  QString errorMessage;
  InstrumentDatabase database;
};

//---------------------------------------
class SettingsController : public QObject
//---------------------------------------
{
  Q_OBJECT

  Q_PROPERTY(QString midiInPort
    READ midiInPort
    WRITE setMidiInPort
    NOTIFY midiInPortChanged)

  Q_PROPERTY(QString midiOutPort
    READ midiOutPort
    WRITE setMidiOutPort
    NOTIFY midiOutPortChanged)

  Q_PROPERTY(bool filterPresetControlChanges
    READ filterPresetControlChanges
    WRITE setFilterPresetControlChanges
    NOTIFY filterPresetControlChangesChanged)

  Q_PROPERTY(int midiInChannel
    READ midiInChannel
    WRITE setMidiInChannel
    NOTIFY midiInChannelChanged)

  Q_PROPERTY(QStringList midiInputPorts
    READ midiInputPorts
    NOTIFY midiInputPortsChanged)

  Q_PROPERTY(QStringList midiOutputPorts
    READ midiOutputPorts
    NOTIFY midiOutputPortsChanged)

  Q_PROPERTY(int patchPolicy
    READ patchPolicy
    WRITE setPatchPolicy
    NOTIFY patchPolicyChanged)

  Q_PROPERTY(bool useInstrumentDefinition
    READ useInstrumentDefinition
    WRITE setUseInstrumentDefinition
    NOTIFY useInstrumentDefinitionChanged)

  Q_PROPERTY(int keyboardRangeId
    READ keyboardRangeId
    WRITE setKeyboardRangeId
    NOTIFY keyboardRangeIdChanged)

  Q_PROPERTY(int pitchBendRange
    READ pitchBendRange
    WRITE setPitchBendRange
    NOTIFY pitchBendRangeChanged)

  Q_PROPERTY(int playMode
    READ playMode
    WRITE setPlayMode
    NOTIFY playModeChanged)

  Q_PROPERTY(QStringList instrumentNames
    READ instrumentNames
    NOTIFY instrumentNamesChanged)

  Q_PROPERTY(QString knownInstrumentName
    READ knownInstrumentName
    WRITE setKnownInstrumentName
    NOTIFY knownInstrumentNameChanged)

  Q_PROPERTY(bool instrumentDatabaseLoading
    READ instrumentDatabaseLoading
    NOTIFY instrumentDatabaseLoadingChanged)

  Q_PROPERTY(QString instrumentDatabaseError
    READ instrumentDatabaseError
    NOTIFY instrumentDatabaseErrorChanged)

  Q_PROPERTY(int surfaceMinNote
    READ surfaceMinNote
    NOTIFY surfaceKeyboardRangeChanged)

  Q_PROPERTY(int surfaceMaxNote
    READ surfaceMaxNote
    NOTIFY surfaceKeyboardRangeChanged)

  Q_PROPERTY(QStringList surfaceMorphOutputTrackTexts
    READ surfaceMorphOutputTrackTexts
    NOTIFY surfaceMorphOutputsChanged)

  Q_PROPERTY(QVariantList surfaceMorphOutputTrackMasks
    READ surfaceMorphOutputTrackMasks
    NOTIFY surfaceMorphOutputsChanged)

  Q_PROPERTY(int morphOutputMuteMask READ morphOutputMuteMask NOTIFY morphOutputMuteSoloChanged)
  Q_PROPERTY(int morphOutputSoloMask READ morphOutputSoloMask NOTIFY morphOutputMuteSoloChanged)

  Q_PROPERTY(QAbstractItemModel* morphOutputStateModel
    READ morphOutputStateModel
    CONSTANT)

  Q_PROPERTY(QStringList presetNames
    READ presetNames
    NOTIFY presetNamesChanged)

  Q_PROPERTY(int currentPresetIndex
    READ currentPresetIndex
    WRITE setCurrentPresetIndex
    NOTIFY currentPresetIndexChanged)

  Q_PROPERTY(QString currentPresetName
    READ currentPresetName
    NOTIFY currentPresetNameChanged)

  Q_PROPERTY(QString currentPresetNotes
    READ currentPresetNotes
    WRITE setCurrentPresetNotes
    NOTIFY currentPresetNotesChanged)

  Q_PROPERTY(QString aboutHtml READ aboutHtml CONSTANT)
  Q_PROPERTY(QVariantList userManualBlocks READ userManualBlocks CONSTANT)

public:
  explicit SettingsController(QObject* parent = nullptr);
  ~SettingsController() override;

  QString midiInPort() const;
  void setMidiInPort(const QString& portName);

  QString midiOutPort() const;
  void setMidiOutPort(const QString& portName);

  bool filterPresetControlChanges() const;
  void setFilterPresetControlChanges(bool enabled);

  int midiInChannel() const;
  void setMidiInChannel(int channel);

  QStringList midiInputPorts() const;
  QStringList midiOutputPorts() const;

  PresetData& currentPreset();
  const PresetData& currentPreset() const;

  int patchPolicy() const;
  void setPatchPolicy(int value);

  bool useInstrumentDefinition() const;
  void setUseInstrumentDefinition(bool enabled);

  int keyboardRangeId() const;
  void setKeyboardRangeId(int value);

  int pitchBendRange() const;
  void setPitchBendRange(int value);

  int playMode() const;
  void setPlayMode(int value);
  bool loadInstrumentDatabase(const QString& filePath, QString* errorMessage = nullptr);

  QStringList instrumentNames() const;
  Q_INVOKABLE QStringList findInstrumentNames(const QString& nameFilter) const;

  QString knownInstrumentName() const;
  void setKnownInstrumentName(const QString& name);

  const InstrumentDatabase& instrumentDatabase() const { return instrumentDatabase_; }
  const InstrumentDefinition* currentInstrumentDefinition() const;

  void loadInstrumentDatabaseAsync(const QString& filePath);

  bool instrumentDatabaseLoading() const;
  QString instrumentDatabaseError() const;

  bool sendTrackControlChange(int trackIndex, uint8_t cc, uint8_t value);
  bool sendTrackProgramChange(int trackIndex, uint8_t program);
  bool sendTrackBankSelectAndProgram(int trackIndex, uint8_t bankMSB, uint8_t bankLSB, uint8_t program);

  int surfaceMinNote() const;
  int surfaceMaxNote() const;

  QStringList surfaceMorphOutputTrackTexts() const;
  void notifySurfaceMorphOutputsChanged(int trackIndex);
  void initializeMorphOutputNameIfNeeded(int morphOutputIndex, int trackIndex);
  QVariantList surfaceMorphOutputTrackMasks() const;
  QAbstractItemModel* morphOutputStateModel() const;
  int morphOutputMuteMask() const;
  int morphOutputSoloMask() const;

  void notifyMidiRelevantStateChanged();
  QStringList presetNames() const;
  Q_INVOKABLE QStringList findPresetNames(const QString& nameFilter) const;

  int currentPresetIndex() const;
  void setCurrentPresetIndex(int index);

  QString currentPresetName() const;
  QString currentPresetNotes() const;
  void setCurrentPresetNotes(const QString& notes);

  QString aboutHtml() const;
  QVariantList userManualBlocks() const;

  Q_INVOKABLE void refreshMidiInPorts();
  Q_INVOKABLE void refreshMidiOutPorts();
  Q_INVOKABLE void delayedMidiRefreshAfterStartup();
  Q_INVOKABLE QObject* track(int trackNumber) const;
  Q_INVOKABLE QObject* keyCurve() const;
  Q_INVOKABLE QObject* velocityCurve() const;
  Q_INVOKABLE void surfacePress(double xNorm, double yNorm);
  Q_INVOKABLE void surfaceMove(double xNorm, double yNorm);
  Q_INVOKABLE void surfaceRelease();
  Q_INVOKABLE void testNoteOn(int note, int velocity);
  Q_INVOKABLE void testNoteOff(int note);
  Q_INVOKABLE QVariantList currentMorphOutputGains() const;
  Q_INVOKABLE QVariantList currentMonitorFeedbackNotes() const;
  Q_INVOKABLE void setSurfaceMorphOutputTrackMask(int morphOutputIndex, int mask);
  Q_INVOKABLE void setMorphOutputMuted(int morphOutputIndex, bool muted);
  Q_INVOKABLE void setMorphOutputSolo(int morphOutputIndex, bool solo);
  Q_INVOKABLE void setMorphOutputName(int morphOutputIndex, const QString& name);
  Q_INVOKABLE void sendGM2Reset();
  Q_INVOKABLE void sendSoftReset(int channel);
  Q_INVOKABLE bool saveCurrentPreset();
  Q_INVOKABLE bool saveCurrentPresetAs(const QString& name);
  Q_INVOKABLE bool deleteCurrentPreset();
  Q_INVOKABLE bool presetNameExists(const QString& name) const;
  Q_INVOKABLE void activatePreset(int index);

private:
  void loadAppInitSettings();
  void saveAppInitSettings() const;
  void refreshMidiPorts();
  void sendCurrentPresetSetupToMidiEngine();

  void startMidiEngine();
  void stopMidiEngine();
  MidiEngineSnapshot makeMidiEngineSnapshot() const;
  void syncMidiEngineSnapshot();
  void applyPreset(const PresetData& preset);
  void emitCurrentPresetStateChanged();
  QString automaticTrackProgramName(int trackIndex) const;

signals:
  void midiInPortChanged();
  void midiOutPortChanged();
  void filterPresetControlChangesChanged();
  void midiInChannelChanged();
  void midiInputPortsChanged();
  void midiOutputPortsChanged();
  void patchPolicyChanged();
  void useInstrumentDefinitionChanged();
  void keyboardRangeIdChanged();
  void pitchBendRangeChanged();
  void playModeChanged();
  void instrumentNamesChanged();
  void knownInstrumentNameChanged();
  void instrumentDatabaseLoadingChanged();
  void instrumentDatabaseErrorChanged();
  void surfaceKeyboardRangeChanged();
  void surfaceMorphOutputsChanged();
  void morphOutputMuteSoloChanged();
  void monitorFeedbackChanged(QVariantList notes, QVariantList gains);
  void midiInputReceivedWithNoAssignedTracks();
  void presetNamesChanged();
  void currentPresetIndexChanged();
  void currentPresetNameChanged();
  void currentPresetNotesChanged();

private:
  QThread* midiThread_ = nullptr;
  MidiEngine* midiEngine_ = nullptr;
  MorphOutputStateModel* morphOutputStateModel_ = nullptr;

  QStringList midiInputPorts_;
  QStringList midiOutputPorts_;

  PresetData currentPreset_;

  std::array<TrackController*, 16> tracks_{};

  InstrumentDatabase instrumentDatabase_;
  bool instrumentDatabaseLoading_ = false;
  QString instrumentDatabaseError_;

  CurveController* keyCurveController;
  CurveController* velocityCurveController;

  QVariantList morphOutputGains_;
  QVariantList monitorFeedbackNotes_;

  PresetManager presetManager_;
  int currentPresetIndex_ = -1;
};