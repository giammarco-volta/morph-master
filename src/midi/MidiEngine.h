#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QTimer>
#include <QElapsedTimer>

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

#include "../core/Presets.h"

class IMidiOut;
class MidiIn_MonoInterpreter;
struct MidiInEvent;

struct MidiEngineSnapshot
{
  QString midiInPort;
  QString midiOutPort;
  uint8_t midiInChannel = 0;
  bool filterPresetControlChanges = true;

  PlayMode playMode = PlayMode::Poly;
  uint8_t pitchBendRange = 2;
  uint8_t morphOutputMuteMask = 0;
  uint8_t morphOutputSoloMask = 0;

  int surfaceMinNote = 0;
  int surfaceMaxNote = 127;

  std::array<TrackPresetData, 16> tracks{};
  PiecewiseCurve keyCurve;
  PiecewiseCurve velCurve;
};

class MidiEngine : public QObject
{
  Q_OBJECT

public:
  explicit MidiEngine(QObject* parent = nullptr);
  ~MidiEngine() override;

  void sendAllNotesOffAndExpressionReset(int trackIndex);
  void sendAllNotesOffAndExpressionReset();

private:
  bool openCurrentMidiIn(bool forceReopen);
  bool openCurrentMidiOut(bool forceReopen);

  void handleIncomingMidiEvent(const MidiInEvent& ev);
  void handleIncomingNoteOn(uint8_t note, uint8_t velocity);
  void handleIncomingNoteOff(uint8_t note, uint8_t velocity);
  void handleIncomingControlChange(uint8_t controller, uint8_t value);
  void handleIncomingPitchBend(uint8_t lsb, uint8_t msb);
  void handleIncomingChannelPressure(uint8_t pressure);
  void handleIncomingPolyPressure(uint8_t note, uint8_t pressure);

  void prepareDetuneForNote(uint8_t velocity);
  void randomizeAndApplyTrackTunings(uint8_t velocity);
  void applyTrackTuning(int trackIndex);
  void applyBaseTrackTunings(bool includeUnassignedTracks);
  int totalTrackTuningCents(int trackIndex) const;
  int spreadDetuneCents(int trackIndex, uint8_t velocity) const;
  void resetPerformanceState();
  void updatePhraseStateAfterNoteOff();
  void forwardControlChangeToAssignedTracks(uint8_t controller, uint8_t value);
  void forwardPitchBendToAssignedTracks(uint8_t lsb, uint8_t msb);
  void applyPitchBendRangeToAssignedTracks(bool includeUnassignedTracks);
  void sendPitchBendRange(int trackIndex, int semitones);

  static constexpr int MorphOutputCount = static_cast<int>(MorphOutputId::None);
  using MorphOutputGains = std::array<double, MorphOutputCount>;

  MorphOutputGains calculateMorphOutputGains(uint8_t note, uint8_t velocity) const;
  void registerMorphNoteOn(uint8_t note, const MorphOutputGains& gains);
  void registerMorphNoteOff(uint8_t note);
  void releaseSustainedMorphNotes();
  void recomputeDisplayedMorphOutputGains();
  void clearMorphOutputGainState();

  void registerMonitorNoteOn(uint8_t note, uint8_t velocity);
  void registerMonitorNoteOff(uint8_t note);
  void publishMonitorFeedback();
  void clearMonitorFeedbackState();

  bool sendTrackNoteOn(int trackIndex, uint8_t note, uint8_t velocity);
  bool sendTrackNoteOff(int trackIndex, uint8_t note, uint8_t velocity);

  uint8_t noteFromNormalized(double xNorm) const;
  uint8_t velocityFromNormalized(double yNorm) const;

  bool hasHeldNotes() const;
  bool hasSoundingNotes() const;
  void onMidiWatchdog();

  bool hasAnyAssignedTrack() const;
  bool isMorphOutputEnabled(MorphOutputId output) const;

public slots:
  void start();
  void stop();

  void setSnapshot(const MidiEngineSnapshot& snapshot);

  void refreshMidiPorts();

  void sendTrackControlChange(int trackIndex, int cc, int value);
  void sendTrackProgramChange(int trackIndex, int program);
  void sendTrackBankSelectAndProgram(int trackIndex, int bankMSB, int bankLSB, int program);
  void sendCurrentPresetTrackSetup();
  void sendRNPFineTuning(int trackIndex, int cents);

  void sendGM2Reset();
  void sendSoftReset(int channel1Based);

  void surfacePress(double xNorm, double yNorm);
  void surfaceMove(double xNorm, double yNorm);
  void surfaceRelease();

  void testNoteOn(int note, int velocity);
  void testNoteOff(int note);
  void acknowledgeMonitorFeedback(quint64 sequence);

signals:
  void midiInputPortsChanged(QStringList ports);
  void midiOutputPortsChanged(QStringList ports);

  void midiInputReceivedWithNoAssignedTracks();

  void monitorFeedbackSnapshot(QVariantList notes,
                               QVariantList gains,
                               quint64 sequence);

  void midiInPortResolved(QString portName);
  void midiOutPortResolved(QString portName);

private:
  std::unique_ptr<MidiIn_MonoInterpreter> midiIn_;
  std::unique_ptr<IMidiOut> midiOut_;

  QStringList midiInputPorts_;
  QStringList midiOutputPorts_;

  int midiInIndex_ = -1;
  int midiOutIndex_ = -1;

  QString midiInPort_;
  QString midiOutPort_;

  MidiEngineSnapshot snapshot_;

  std::list<uint8_t> currNotes_;
  std::array<bool, 128> activeInputNotes_{};
  QElapsedTimer lastMidiInActivity_;
  QTimer* midiWatchdogTimer_ = nullptr;

  std::array<uint8_t, 16> last_cc71_{};
  std::array<uint8_t, 16> last_cc74_{};

  std::array<int, 16> currentRandomDetuneCents_{};
  std::array<int, 16> lastSentFineTuningCents_{};

  struct ActiveMorphNote
  {
    bool held = false;
    bool sounding = false;
    MorphOutputGains gains{};
  };

  std::array<ActiveMorphNote, 128> activeMorphNotes_{};
  MorphOutputGains displayedMorphOutputGains_{};

  struct MonitorFeedbackNote
  {
    bool active = false;
    uint8_t velocity = 0;
    quint64 onGeneration = 0;
    quint64 acknowledgedOnGeneration = 0;
    qint64 retainUntilMs = 0;
  };

  static constexpr int MonitorFeedbackIntervalMs = 25;
  static constexpr int MonitorFeedbackMinVisibleMs = 50;

  std::array<MonitorFeedbackNote, 128> monitorFeedbackNotes_{};
  std::array<quint64, 128> publishedMonitorNoteGenerations_{};
  MorphOutputGains pendingPeakMorphOutputGains_{};
  QTimer* monitorFeedbackTimer_ = nullptr;
  QElapsedTimer monitorFeedbackClock_;
  bool monitorFeedbackDirty_ = true;
  bool monitorFeedbackSnapshotPending_ = false;
  quint64 monitorFeedbackSequence_ = 0;
  quint64 pendingMonitorFeedbackSequence_ = 0;

  bool sustainDown_ = false;
  uint8_t sustainValue_ = 0;
  bool phraseActive_ = false;

  bool surfaceTestNoteActive_ = false;
  uint8_t surfaceTestNote_ = 0;
  uint8_t surfaceTestVelocity_ = 0;
  std::array<bool, 128> activeTestNotes_{};

  bool noAssignedTracksWarningSent_ = false;
};