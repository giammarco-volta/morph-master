#include "MidiEngine.h"

#include "IMidiOut.h"
#include "MidiInFactory.h"
#include "MidiOutFactory.h"
#include "MidiMessage.h"
#include "MidiMonoIn.h"

#include "../core/ExpressionCalculator.h"

#include <QDebug>
#include <QMetaObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QVariantMap>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>

namespace
{
  //-------------------------
  uint8_t midi7bit(int value)
    //-------------------------
  {
    return static_cast<uint8_t>(std::clamp(value, 0, 127));
  }

  //----------------------------------------------------
  bool isPresetSetupControlChange(uint8_t controller)
  //----------------------------------------------------
  {
    switch (controller)
    {
      case 7:   // Channel Volume
      case 10:  // Pan
      case 11:  // Expression
      case 71:  // Resonance / Tone
      case 74:  // Brightness / Timbre
      case 91:  // Reverb Send
      case 93:  // Chorus Send
        return true;

      default:
        return false;
    }
  }

  //------------------------------------------------------------------
  uint8_t timbreControllerValue(int contribution, uint8_t expression)
  //------------------------------------------------------------------
  {
    const double value = 64.0
      + (static_cast<double>(expression) / 2.0)
      * (static_cast<double>(contribution) / 64.0);

    return static_cast<uint8_t>(std::clamp(static_cast<int>(value),
                                           0, 127));
  }

  //------------------------------------------
  static QString stableMidiPortName(QString s)
    //------------------------------------------
  {
    s = s.trimmed().simplified();

#ifdef Q_OS_ANDROID
    // Android can generate names like:
    // "Korg Inc. Pa5X#748 MIDI 1.0"
    // "Korg Inc. Pa5X#366 MIDI 1.0"
    //
    // The number after # is volatile and changes after USB power-cycle.
    // Remove it wherever it is in the string, not only at the end.
    s.remove(QRegularExpression(QStringLiteral("#\\d+")));
#endif

    return s.trimmed().simplified();
  }

  //----------------------------------------------------------------
  static QStringList stableMidiPortNames(const QStringList& rawPorts)
    //----------------------------------------------------------------
  {
    QStringList result;
    result.reserve(rawPorts.size());

    for (const auto& port : rawPorts)
      result << stableMidiPortName(port);

    return result;
  }

  //----------------------------------------------------------------------------------------
  static int findMidiPortIndex(const QStringList& rawPorts, const QString& wantedStableName)
    //----------------------------------------------------------------------------------------
  {
    const QString wantedTrimmed = wantedStableName.trimmed();

    if (wantedTrimmed.isEmpty())
      return -1;

    // First try an exact match on the raw name. This is useful when the saved
    // value still contains #xyz and the device has not been power-cycled.
    const int exactIndex = rawPorts.indexOf(wantedTrimmed);

    if (exactIndex >= 0)
      return exactIndex;

    // Then try a stable match: the saved/displayed name must not depend on
    // Android's volatile #xyz identifier.
    const QString wantedStable = stableMidiPortName(wantedTrimmed);

    QList<int> matches;

    for (int i = 0; i < rawPorts.size(); ++i)
    {
      const QString portStable = stableMidiPortName(rawPorts[i]);

      if (portStable.compare(wantedStable, Qt::CaseInsensitive) == 0)
        matches << i;
    }

    if (matches.size() == 1)
      return matches.first();

    if (matches.size() > 1)
      qWarning() << "Ambiguous MIDI port name" << wantedStable << "matches" << matches.size() << "ports";

    return -1;
  }
}

//-------------------------------------------------------
MidiEngine::MidiEngine(QObject* parent) : QObject(parent)
//-------------------------------------------------------
{
  last_cc71_.fill(64);
  last_cc74_.fill(64);
  currentRandomDetuneCents_.fill(0);
  lastSentFineTuningCents_.fill(1000);
}

//-----------------------
MidiEngine::~MidiEngine()
//-----------------------
{
  stop();
}

//----------------------
void MidiEngine::start()
//----------------------
{
  midiOut_ = createMidiOut();
  midiIn_ = createMidiIn();

  if (midiIn_)
  {
    midiIn_->setSourceChannel(snapshot_.midiInChannel);

    midiIn_->setCallback([this](const MidiInEvent& ev)
      {
        QMetaObject::invokeMethod(
          this,
          [this, ev]()
          {
            handleIncomingMidiEvent(ev);
          },
          Qt::QueuedConnection);
      });
  }
  else
  {
    qWarning() << "MIDI IN backend not available";
  }

  if (!midiOut_)
    qWarning() << "MIDI OUT backend not available";

  refreshMidiPorts();
  lastMidiInActivity_.start();

  monitorFeedbackClock_.start();
  monitorFeedbackTimer_ = new QTimer(this);
  monitorFeedbackTimer_->setInterval(MonitorFeedbackIntervalMs);
  monitorFeedbackTimer_->setTimerType(Qt::PreciseTimer);
  connect(monitorFeedbackTimer_, &QTimer::timeout,
          this, &MidiEngine::publishMonitorFeedback);
  monitorFeedbackTimer_->start();

#ifdef Q_OS_ANDROID
  midiWatchdogTimer_ = new QTimer(this);
  midiWatchdogTimer_->setInterval(30000);

  connect(midiWatchdogTimer_, &QTimer::timeout, this, &MidiEngine::onMidiWatchdog);

  midiWatchdogTimer_->start();
#endif
}

//---------------------
void MidiEngine::stop()
//---------------------
{
  if (monitorFeedbackTimer_)
  {
    monitorFeedbackTimer_->stop();
    monitorFeedbackTimer_->deleteLater();
    monitorFeedbackTimer_ = nullptr;
  }

#ifdef Q_OS_ANDROID
  if (midiWatchdogTimer_)
  {
    midiWatchdogTimer_->stop();
    midiWatchdogTimer_->deleteLater();
    midiWatchdogTimer_ = nullptr;
  }
#endif

  if (midiIn_)
    midiIn_->close();

  if (midiOut_)
    midiOut_->close();

  midiIn_.reset();
  midiOut_.reset();

  midiInIndex_ = -1;
  midiOutIndex_ = -1;
  midiInPort_.clear();
  midiOutPort_.clear();

  resetPerformanceState();
  surfaceTestNoteActive_ = false;
}

//--------------------------------------------------------------
void MidiEngine::setSnapshot(const MidiEngineSnapshot& snapshot)
//--------------------------------------------------------------
{
  const bool midiInPortChanged = snapshot_.midiInPort != snapshot.midiInPort;
  const bool midiOutPortChanged = snapshot_.midiOutPort != snapshot.midiOutPort;
  const bool midiInChannelChanged = snapshot_.midiInChannel != snapshot.midiInChannel;
  const bool playModeChanged = snapshot_.playMode != snapshot.playMode;
  const bool pitchBendRangeChanged = snapshot_.pitchBendRange != snapshot.pitchBendRange;
  const bool muteSoloChanged = snapshot_.morphOutputMuteMask != snapshot.morphOutputMuteMask
                            || snapshot_.morphOutputSoloMask != snapshot.morphOutputSoloMask;

  std::array<bool, 16> tuningSettingsChanged{};
  std::array<bool, 16> morphOutputChanged{};
  bool anyTuningSettingsChanged = false;

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    const auto& oldTrack = snapshot_.tracks[trackIndex];
    const auto& newTrack = snapshot.tracks[trackIndex];

    tuningSettingsChanged[trackIndex] =
      oldTrack.footage != newTrack.footage
      || oldTrack.detuneOffset != newTrack.detuneOffset
      || oldTrack.detuneSpread != newTrack.detuneSpread;

    morphOutputChanged[trackIndex] = oldTrack.morphOutput != newTrack.morphOutput;

    anyTuningSettingsChanged = anyTuningSettingsChanged || tuningSettingsChanged[trackIndex];
  }

  const auto oldSnapshot = snapshot_;

  /* Install the new snapshot before sending MIDI derived from it. */
  snapshot_ = snapshot;

  if (muteSoloChanged)
  {
    for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
    {
      const auto oldOutput = oldSnapshot.tracks[trackIndex].morphOutput;
      if (oldOutput == MorphOutputId::None)
        continue;

      const int outputIndex = static_cast<int>(oldOutput);
      const bool oldSoloActive = oldSnapshot.morphOutputSoloMask != 0;
      const bool oldEnabled = (oldSnapshot.morphOutputMuteMask & (1u << outputIndex)) == 0
                           && (!oldSoloActive || (oldSnapshot.morphOutputSoloMask & (1u << outputIndex)) != 0);
      const bool newEnabled = isMorphOutputEnabled(oldOutput);

      if (oldEnabled && !newEnabled)
        sendAllNotesOffAndExpressionReset(trackIndex);
    }
  }

  if (midiIn_ && midiInChannelChanged)
    midiIn_->setSourceChannel(snapshot_.midiInChannel);

  if (midiInPortChanged || midiInChannelChanged)
    openCurrentMidiIn(false);

  if (midiOutPortChanged)
    openCurrentMidiOut(false);

  if (midiOutPortChanged)
  {
    /* A new device has no knowledge of our channel tuning. */
    resetPerformanceState();
    lastSentFineTuningCents_.fill(1000);
    applyBaseTrackTunings(true);
    applyPitchBendRangeToAssignedTracks(true);
  }
  else if (playModeChanged
           || midiInChannelChanged
           || anyTuningSettingsChanged)
  {
    /* Fine tuning is channel-wide: never change it under sounding notes. */
    sendAllNotesOffAndExpressionReset();
    applyBaseTrackTunings(false);
  }
  else
  {
    /* Assignment changes only affect the corresponding track. */
    for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
    {
      if (!morphOutputChanged[trackIndex])
        continue;

      currentRandomDetuneCents_[trackIndex] = 0;

      if (snapshot_.tracks[trackIndex].morphOutput == MorphOutputId::None)
        continue;

      applyTrackTuning(trackIndex);
      sendPitchBendRange(trackIndex, snapshot_.pitchBendRange);

      if (sustainDown_)
        sendTrackControlChange(trackIndex, 64, sustainValue_);
    }
  }

  if (pitchBendRangeChanged && !midiOutPortChanged)
    applyPitchBendRangeToAssignedTracks(false);

  if (hasAnyAssignedTrack())
    noAssignedTracksWarningSent_ = false;
}

//---------------------------------
void MidiEngine::refreshMidiPorts()
//---------------------------------
{
  QStringList newInputPorts;
  QStringList newOutputPorts;

  if (midiIn_)
    newInputPorts = midiIn_->listInputs();

  if (midiOut_)
    newOutputPorts = midiOut_->listOutputs();

  if (midiInputPorts_ != newInputPorts)
  {
    midiInputPorts_ = newInputPorts;
    emit midiInputPortsChanged(stableMidiPortNames(midiInputPorts_));
  }

  if (midiOutputPorts_ != newOutputPorts)
  {
    midiOutputPorts_ = newOutputPorts;
    emit midiOutputPortsChanged(stableMidiPortNames(midiOutputPorts_));
  }

  openCurrentMidiIn(true);

  if (openCurrentMidiOut(true))
  {
    lastSentFineTuningCents_.fill(1000);
    applyBaseTrackTunings(true);
    applyPitchBendRangeToAssignedTracks(true);
  }

  qDebug() << "MORPHMASTER Saved MIDI IN:" << snapshot_.midiInPort;
  qDebug() << "MORPHMASTER Available MIDI IN raw:" << midiInputPorts_;
  qDebug() << "MORPHMASTER Available MIDI IN stable:" << stableMidiPortNames(midiInputPorts_);

  qDebug() << "MORPHMASTER Saved MIDI OUT:" << snapshot_.midiOutPort;
  qDebug() << "MORPHMASTER Available MIDI OUT raw:" << midiOutputPorts_;
  qDebug() << "MORPHMASTER Available MIDI OUT stable:" << stableMidiPortNames(midiOutputPorts_);
}

//--------------------------------------------------
bool MidiEngine::openCurrentMidiIn(bool forceReopen)
//--------------------------------------------------
{
  if (!midiIn_)
    return false;

  midiIn_->setSourceChannel(snapshot_.midiInChannel);

  const QString wantedPort = snapshot_.midiInPort.trimmed();

  if (wantedPort.isEmpty())
    return false;

  const int index = findMidiPortIndex(midiInputPorts_, wantedPort);

  if (index < 0)
  {
    midiIn_->close();
    midiInIndex_ = -1;
    midiInPort_.clear();
    return false;
  }

  const QString actualRawPort = midiInputPorts_[index];
  const QString actualStablePort = stableMidiPortName(actualRawPort);
  const bool needsResolvedName = wantedPort != actualStablePort;

  // To check whether the already-open port is the right one, use the raw name:
  // MIDI opening is by index in the raw Android list.
  if (!forceReopen && midiInIndex_ == index && midiInPort_ == actualRawPort)
  {
    if (needsResolvedName)
      emit midiInPortResolved(actualStablePort);

    return true;
  }

  midiIn_->close();

  if (!midiIn_->open(index))
  {
    qWarning() << "MIDI IN open failed:" << actualStablePort << "raw:" << actualRawPort;

    midiInIndex_ = -1;
    midiInPort_.clear();
    return false;
  }

  midiInIndex_ = index;
  midiInPort_ = actualRawPort;

  if (needsResolvedName)
    emit midiInPortResolved(actualStablePort);

  qDebug() << "MIDI IN connected:" << actualStablePort << "raw:" << actualRawPort;

  return true;
}

//---------------------------------------------------
bool MidiEngine::openCurrentMidiOut(bool forceReopen)
//---------------------------------------------------
{
  if (!midiOut_)
    return false;

  const QString wantedPort = snapshot_.midiOutPort.trimmed();

  if (wantedPort.isEmpty())
    return false;

  const int index = findMidiPortIndex(midiOutputPorts_, wantedPort);

  if (index < 0)
  {
    midiOut_->close();
    midiOutIndex_ = -1;
    midiOutPort_.clear();
    return false;
  }

  const QString actualRawPort = midiOutputPorts_[index];
  const QString actualStablePort = stableMidiPortName(actualRawPort);
  const bool needsResolvedName = wantedPort != actualStablePort;

  // To check whether the already-open port is the right one, use the raw name:
  // MIDI opening is by index in the raw Android list.
  if (!forceReopen && midiOutIndex_ == index && midiOutPort_ == actualRawPort)
  {
    if (needsResolvedName)
      emit midiOutPortResolved(actualStablePort);

    return true;
  }

  midiOut_->close();

  if (!midiOut_->open(index))
  {
    qWarning() << "MIDI OUT open failed:" << actualStablePort << "raw:" << actualRawPort;

    midiOutIndex_ = -1;
    midiOutPort_.clear();
    return false;
  }

  midiOutIndex_ = index;
  midiOutPort_ = actualRawPort;
  lastSentFineTuningCents_.fill(1000);

  if (needsResolvedName)
    emit midiOutPortResolved(actualStablePort);

  qDebug() << "MIDI OUT connected:" << actualStablePort << "raw:" << actualRawPort;

  return true;
}

//-------------------------------------------------------------
void MidiEngine::handleIncomingMidiEvent(const MidiInEvent& ev)
//-------------------------------------------------------------
{
  lastMidiInActivity_.restart();

  if (ev.type != MidiInEvent::Type::ShortMessage)
    return;

  const uint8_t message = ev.message();

  if (message == 0xD0)
  {
    handleIncomingChannelPressure(ev.data1);
    return;
  }

  if (message == 0xA0)
  {
    handleIncomingPolyPressure(ev.data1, ev.data2);
    return;
  }

  if (message == 0xB0)
  {
    handleIncomingControlChange(ev.data1, ev.data2);
    return;
  }

  if (message == 0xE0)
  {
    handleIncomingPitchBend(ev.data1, ev.data2);
    return;
  }

  if (message == 0x90 && ev.data2 != 0)
  {
    const uint8_t note = ev.data1;
    activeInputNotes_[note] = true;

    if (snapshot_.playMode == PlayMode::Poly)
    {
      prepareDetuneForNote(ev.data2);
      handleIncomingNoteOn(note, ev.data2);
      return;
    }

    const bool hadSoundingNote = !currNotes_.empty();
    const uint8_t previousSoundingNote =
      hadSoundingNote ? currNotes_.back() : 0;

    currNotes_.remove(note);

    /* Retune only after the previous mono note has been switched off. */
    if (hadSoundingNote && previousSoundingNote != note)
      handleIncomingNoteOff(previousSoundingNote, ev.data2);

    prepareDetuneForNote(ev.data2);
    handleIncomingNoteOn(note, ev.data2);
    currNotes_.push_back(note);
    return;
  }

  if (message == 0x80 || (message == 0x90 && ev.data2 == 0))
  {
    const uint8_t note = ev.data1;
    activeInputNotes_[note] = false;

    if (snapshot_.playMode == PlayMode::Poly)
    {
      handleIncomingNoteOff(note, ev.data2);
      updatePhraseStateAfterNoteOff();
      return;
    }

    if (!currNotes_.empty() && note == currNotes_.back())
    {
      handleIncomingNoteOff(note, ev.data2);
      currNotes_.remove(note);

      if (!currNotes_.empty())
      {
        prepareDetuneForNote(ev.data2 != 0 ? ev.data2 : 64);
        handleIncomingNoteOn(
          currNotes_.back(),
          ev.data2 != 0 ? ev.data2 : 64);
      }
    }
    else
    {
      currNotes_.remove(note);
    }

    updatePhraseStateAfterNoteOff();
  }
}

//-----------------------------------------------------------------------------
void MidiEngine::handleIncomingControlChange(uint8_t controller, uint8_t value)
//-----------------------------------------------------------------------------
{
  /*
   * MidiIn_MonoInterpreter has already filtered Program Change,
   * Bank Select (CC0/CC32) and RPN/NRPN parameter controls.
   *
   * The user may additionally protect the controller values belonging to
   * the preset setup. Expression is always reserved for MorphMaster in Mono
   * mode because CC11 performs the real-time crossfade between outputs.
   */
  const bool expressionReservedByMonoMode =
    controller == 11 && snapshot_.playMode == PlayMode::Mono;

  const bool filteredByPresetProtection =
    snapshot_.filterPresetControlChanges
    && isPresetSetupControlChange(controller);

  if (!expressionReservedByMonoMode && !filteredByPresetProtection)
    forwardControlChangeToAssignedTracks(controller, value);

  /*
   * Sustain always affects MorphMaster's internal note/phrase state. CC64
   * is not a preset setup controller, so it is also always forwarded.
   */
  if (controller == 64)
  {
    const bool wasDown = sustainDown_;
    sustainValue_ = value;
    sustainDown_ = value >= 64;

    if (wasDown && !sustainDown_)
    {
      releaseSustainedMorphNotes();
      updatePhraseStateAfterNoteOff();
    }
  }
}

//----------------------------------------------------------------
void MidiEngine::handleIncomingPitchBend(uint8_t lsb, uint8_t msb)
//----------------------------------------------------------------
{
  forwardPitchBendToAssignedTracks(lsb, msb);
}

//--------------------------------------------------------------
void MidiEngine::handleIncomingChannelPressure(uint8_t pressure)
//--------------------------------------------------------------
{
  if (!openCurrentMidiOut(false))
    return;

  const uint8_t pressureValue = midi7bit(pressure);

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    /*
     * Channel Pressure is a channel-wide performance control.
     * Keep it synchronized on every assigned output track,
     * including tracks temporarily excluded by Mute/Solo.
     */
    if (snapshot_.tracks[trackIndex].morphOutput == MorphOutputId::None)
      continue;

    const uint8_t status = static_cast<uint8_t>(0xD0 | trackIndex);

    /*
     * Channel Pressure has only one data byte.
     * IMidiOut::sendShort still accepts a third argument,
     * which is unused for this MIDI message.
     */
    midiOut_->sendShort(status, pressureValue, 0);
  }
}

//---------------------------------------------------------------------------
void MidiEngine::handleIncomingPolyPressure(uint8_t note, uint8_t pressure)
//---------------------------------------------------------------------------
{
  if (!openCurrentMidiOut(false))
    return;

  const uint8_t pressureValue = midi7bit(pressure);

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    const auto& track = snapshot_.tracks[trackIndex];

    /*
     * Polyphonic pressure refers to an individual sounding note.
     * Route it only to tracks that would currently receive that note.
     */
    if (track.morphOutput == MorphOutputId::None || !isMorphOutputEnabled(track.morphOutput))
    {
      continue;
    }

    const int transposedNote = static_cast<int>(note) + FootageTransposition[static_cast<size_t>(track.footage)];

    /*
     * Do not clamp an out-of-range transposed note: clamping would
     * apply pressure to a different note at MIDI 0 or MIDI 127.
     */
    if (transposedNote < 0 || transposedNote > 127)
      continue;

    const uint8_t status = static_cast<uint8_t>(0xA0 | trackIndex);

    midiOut_->sendShort(status, static_cast<uint8_t>(transposedNote), pressureValue);
  }
}

//-----------------------------------------------------
void MidiEngine::prepareDetuneForNote(uint8_t velocity)
//-----------------------------------------------------
{
  if (phraseActive_)
    return;

  randomizeAndApplyTrackTunings(velocity);
  phraseActive_ = true;
}

//--------------------------------------------------
void MidiEngine::randomizeAndApplyTrackTunings(uint8_t velocity)
//--------------------------------------------------
{
  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    const auto output = snapshot_.tracks[trackIndex].morphOutput;
    if (output == MorphOutputId::None || !isMorphOutputEnabled(output))
      continue;

    currentRandomDetuneCents_[trackIndex] = spreadDetuneCents(trackIndex, velocity);

    applyTrackTuning(trackIndex);
  }
}

//----------------------------------------------
void MidiEngine::applyTrackTuning(int trackIndex)
//----------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return;

  sendRNPFineTuning(trackIndex, totalTrackTuningCents(trackIndex));
}

//---------------------------------------------------------------
void MidiEngine::applyBaseTrackTunings(bool includeUnassignedTracks)
//---------------------------------------------------------------
{
  currentRandomDetuneCents_.fill(0);

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    if (!includeUnassignedTracks
        && snapshot_.tracks[trackIndex].morphOutput == MorphOutputId::None)
    {
      continue;
    }

    applyTrackTuning(trackIndex);
  }
}

//-----------------------------------------------------
int MidiEngine::totalTrackTuningCents(int trackIndex) const
//-----------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return 0;

  const auto& track = snapshot_.tracks[trackIndex];
  const auto footageIndex = static_cast<uint8_t>(track.footage);

  return std::clamp(
    static_cast<int>(FootageDetune[footageIndex])
      + static_cast<int>(track.detuneOffset)
      + currentRandomDetuneCents_[trackIndex],
    -100,
    100);
}

//---------------------------------------------------------------
int MidiEngine::spreadDetuneCents(int trackIndex, uint8_t velocity) const
//---------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return 0;

  const int spread = std::clamp(static_cast<int>(snapshot_.tracks[trackIndex].detuneSpread), 0, 50);

  if (spread == 0)
    return 0;

  /*
   * Half of the spread follows velocity linearly:
   * velocity 1   -> -0.5 * spread
   * velocity 64  ->  0
   * velocity 127 -> +0.5 * spread
   *
   * The other half is uniformly random. Their sum always remains
   * inside the user-facing interval [-spread, +spread].
   */
  const double velocityNorm = (static_cast<double>(std::clamp<int>(velocity, 1, 127)) - 64.0) / 63.0;

  const double velocityComponent = 0.5 * spread * velocityNorm;
  const double randomComponent = spread * (QRandomGenerator::global()->generateDouble() - 0.5);

  return std::clamp(static_cast<int>(std::lround(velocityComponent + randomComponent)), -spread, spread);
}

//--------------------------------------
void MidiEngine::resetPerformanceState()
//--------------------------------------
{
  currNotes_.clear();
  activeInputNotes_.fill(false);
  currentRandomDetuneCents_.fill(0);

  surfaceTestNoteActive_ = false;
  surfaceTestNote_ = 0;
  surfaceTestVelocity_ = 0;
  activeTestNotes_.fill(false);

  sustainDown_ = false;
  sustainValue_ = 0;
  phraseActive_ = false;

  clearMonitorFeedbackState();
  clearMorphOutputGainState();
}

//-----------------------------------------------
void MidiEngine::updatePhraseStateAfterNoteOff()
//-----------------------------------------------
{
  if (!sustainDown_ && !hasHeldNotes())
    phraseActive_ = false;
}

//--------------------------------------------------------------------------------------
void MidiEngine::forwardControlChangeToAssignedTracks(uint8_t controller, uint8_t value)
//--------------------------------------------------------------------------------------
{
  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    /*
     * Keep controller state synchronized on every assigned output track,
     * including tracks currently excluded by Mute/Solo. If they become
     * audible later, their controller state is already up to date.
     */
    if (snapshot_.tracks[trackIndex].morphOutput == MorphOutputId::None)
      continue;

    sendTrackControlChange(trackIndex, controller, value);
  }
}

//---------------------------------------------------------------
void MidiEngine::forwardPitchBendToAssignedTracks(uint8_t lsb, uint8_t msb)
//---------------------------------------------------------------
{
  if (!openCurrentMidiOut(false))
    return;

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    if (snapshot_.tracks[trackIndex].morphOutput == MorphOutputId::None)
      continue;

    const uint8_t status = static_cast<uint8_t>(0xE0 | trackIndex);
    midiOut_->sendShort(status, midi7bit(lsb), midi7bit(msb));
  }
}

//---------------------------------------------------------------
void MidiEngine::applyPitchBendRangeToAssignedTracks(bool includeUnassignedTracks)
//---------------------------------------------------------------
{
  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    if (!includeUnassignedTracks
        && snapshot_.tracks[trackIndex].morphOutput == MorphOutputId::None)
      continue;

    sendPitchBendRange(trackIndex, snapshot_.pitchBendRange);
  }
}

//---------------------------------------------------------------
void MidiEngine::sendPitchBendRange(int trackIndex, int semitones)
//---------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return;

  semitones = std::clamp(semitones, 0, 24);

  // Select Pitch Bend Sensitivity (RPN 0,0).
  sendTrackControlChange(trackIndex, 101, 0);
  sendTrackControlChange(trackIndex, 100, 0);
  sendTrackControlChange(trackIndex, 6, semitones);
  sendTrackControlChange(trackIndex, 38, 0);

  // Deselect the RPN to protect it from later Data Entry messages.
  sendTrackControlChange(trackIndex, 101, 127);
  sendTrackControlChange(trackIndex, 100, 127);
}

//-------------------------------------------------------------------
void MidiEngine::handleIncomingNoteOn(uint8_t note, uint8_t velocity)
//-------------------------------------------------------------------
{
  registerMonitorNoteOn(note, velocity);

  const MorphOutputGains gains = calculateMorphOutputGains(note, velocity);
  registerMorphNoteOn(note, gains);

  if (!hasAnyAssignedTrack())
  {
    if (!noAssignedTracksWarningSent_)
    {
      noAssignedTracksWarningSent_ = true;
      emit midiInputReceivedWithNoAssignedTracks();
    }
    return;
  }

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    const auto& track = snapshot_.tracks[trackIndex];

    if (track.morphOutput == MorphOutputId::None || !isMorphOutputEnabled(track.morphOutput))
      continue;

    const int outputIndex = static_cast<int>(track.morphOutput);
    assert(outputIndex >= 0 && outputIndex < MorphOutputCount);

    const double gain = gains[outputIndex];

    uint8_t scaled_velocity = velocity;
    uint8_t expr = 127;

    if (snapshot_.playMode != PlayMode::Poly)
    {
      expr = static_cast<uint8_t>(std::clamp(
        static_cast<int>(std::lround(127.0 * gain)), 0, 127));
      sendTrackControlChange(trackIndex, 11, expr);
    }
    else
    {
      // In Poly non usiamo expression per miscelare i suoni:
      // usiamo la velocity scalata.
      scaled_velocity = static_cast<uint8_t>(std::clamp(
        static_cast<int>(std::lround(double(velocity) * gain)), 0, 127));

      if (scaled_velocity == 0)
        continue;
    }

    const uint8_t cc71 = timbreControllerValue(track.timbre1, expr);
    const uint8_t cc74 = timbreControllerValue(track.timbre2, expr);

    if (last_cc71_[trackIndex] != cc71)
    {
      last_cc71_[trackIndex] = cc71;
      sendTrackControlChange(trackIndex, 71, cc71);
    }

    if (last_cc74_[trackIndex] != cc74)
    {
      last_cc74_[trackIndex] = cc74;
      sendTrackControlChange(trackIndex, 74, cc74);
    }

    sendTrackNoteOn(trackIndex, note + FootageTransposition[static_cast<size_t>(track.footage)], scaled_velocity);
  }
}

//--------------------------------------------------------------------
void MidiEngine::handleIncomingNoteOff(uint8_t note, uint8_t velocity)
//--------------------------------------------------------------------
{
  registerMonitorNoteOff(note);
  registerMorphNoteOff(note);

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    const auto& track = snapshot_.tracks[trackIndex];

    if (track.morphOutput == MorphOutputId::None || !isMorphOutputEnabled(track.morphOutput))
      continue;

    sendTrackNoteOff(trackIndex, note + FootageTransposition[static_cast<size_t>(track.footage)], velocity);
  }
}

//---------------------------------------------------------------------------------------
MidiEngine::MorphOutputGains MidiEngine::calculateMorphOutputGains(uint8_t note,
                                                                   uint8_t velocity) const
//---------------------------------------------------------------------------------------
{
  MorphOutputGains gains{};

  for (int outputIndex = 0; outputIndex < MorphOutputCount; ++outputIndex)
  {
    const auto outputId = static_cast<MorphOutputId>(outputIndex);
    const MorphOutputProfile profile = MorphOutputProfile::GetProfile(outputId);

    gains[outputIndex] = std::clamp(
      CurveCalculator::computeScaleFactor(
        profile,
        snapshot_.keyCurve,
        snapshot_.velCurve,
        note,
        velocity),
      0.0,
      1.0);
  }

  return gains;
}

//------------------------------------------------------------------------------------
void MidiEngine::registerMorphNoteOn(uint8_t note, const MorphOutputGains& gains)
//------------------------------------------------------------------------------------
{
  auto& state = activeMorphNotes_[note];
  state.held = true;
  state.sounding = true;
  state.gains = gains;

  recomputeDisplayedMorphOutputGains();
}

//-------------------------------------------------------
void MidiEngine::registerMorphNoteOff(uint8_t note)
//-------------------------------------------------------
{
  auto& state = activeMorphNotes_[note];
  state.held = false;

  if (!sustainDown_)
    state.sounding = false;

  recomputeDisplayedMorphOutputGains();
}

//-----------------------------------------------
void MidiEngine::releaseSustainedMorphNotes()
//-----------------------------------------------
{
  for (auto& state : activeMorphNotes_)
  {
    if (!state.held)
      state.sounding = false;
  }

  recomputeDisplayedMorphOutputGains();
}

//-----------------------------------------------------------
void MidiEngine::recomputeDisplayedMorphOutputGains()
//-----------------------------------------------------------
{
  MorphOutputGains newGains{};

  for (const auto& noteState : activeMorphNotes_)
  {
    if (!noteState.sounding)
      continue;

    for (int outputIndex = 0; outputIndex < MorphOutputCount; ++outputIndex)
      newGains[outputIndex] = std::max(newGains[outputIndex], noteState.gains[outputIndex]);
  }

  for (int outputIndex = 0; outputIndex < MorphOutputCount; ++outputIndex)
  {
    const double newGain = newGains[outputIndex];

    if (newGain > pendingPeakMorphOutputGains_[outputIndex])
    {
      pendingPeakMorphOutputGains_[outputIndex] = newGain;
      monitorFeedbackDirty_ = true;
    }

    if (std::abs(newGain - displayedMorphOutputGains_[outputIndex]) < 0.0001)
      continue;

    displayedMorphOutputGains_[outputIndex] = newGain;
    monitorFeedbackDirty_ = true;
  }
}

//--------------------------------------------
void MidiEngine::clearMorphOutputGainState()
//--------------------------------------------
{
  activeMorphNotes_.fill(ActiveMorphNote{});

  bool changed = false;
  for (int outputIndex = 0; outputIndex < MorphOutputCount; ++outputIndex)
  {
    changed = changed
           || std::abs(displayedMorphOutputGains_[outputIndex]) >= 0.0001
           || std::abs(pendingPeakMorphOutputGains_[outputIndex]) >= 0.0001;
    displayedMorphOutputGains_[outputIndex] = 0.0;
    pendingPeakMorphOutputGains_[outputIndex] = 0.0;
  }

  if (changed)
    monitorFeedbackDirty_ = true;
}

//------------------------------------------------------------------------
void MidiEngine::registerMonitorNoteOn(uint8_t note, uint8_t velocity)
//------------------------------------------------------------------------
{
  auto& state = monitorFeedbackNotes_[note];
  state.active = true;
  state.velocity = velocity;
  ++state.onGeneration;
  state.retainUntilMs = 0;
  monitorFeedbackDirty_ = true;
}

//------------------------------------------------------------
void MidiEngine::registerMonitorNoteOff(uint8_t note)
//------------------------------------------------------------
{
  auto& state = monitorFeedbackNotes_[note];
  if (!state.active && state.onGeneration == 0)
    return;

  state.active = false;
  monitorFeedbackDirty_ = true;
}

//--------------------------------------------------
void MidiEngine::clearMonitorFeedbackState()
//--------------------------------------------------
{
  bool changed = false;

  for (auto& state : monitorFeedbackNotes_)
  {
    const bool hadVisualState = state.active
                             || state.onGeneration != state.acknowledgedOnGeneration
                             || state.retainUntilMs > 0;
    changed = changed || hadVisualState;

    if (hadVisualState)
      ++state.onGeneration;

    state.active = false;
    state.velocity = 0;
    state.acknowledgedOnGeneration = state.onGeneration;
    state.retainUntilMs = 0;
  }

  if (changed)
    monitorFeedbackDirty_ = true;
}

//------------------------------------------------
void MidiEngine::publishMonitorFeedback()
//------------------------------------------------
{
  if (monitorFeedbackSnapshotPending_)
    return;

  const qint64 nowMs = monitorFeedbackClock_.isValid()
                     ? monitorFeedbackClock_.elapsed()
                     : 0;

  for (auto& state : monitorFeedbackNotes_)
  {
    if (!state.active
        && state.onGeneration == state.acknowledgedOnGeneration
        && state.retainUntilMs > 0
        && state.retainUntilMs <= nowMs)
    {
      state.retainUntilMs = 0;
      monitorFeedbackDirty_ = true;
    }
  }

  if (!monitorFeedbackDirty_)
    return;

  QVariantList notes;
  publishedMonitorNoteGenerations_.fill(0);

  for (int note = 0; note < static_cast<int>(monitorFeedbackNotes_.size()); ++note)
  {
    const auto& state = monitorFeedbackNotes_[note];
    const bool notYetShown = state.onGeneration > state.acknowledgedOnGeneration;
    const bool retained = state.retainUntilMs > nowMs;

    if (!state.active && !notYetShown && !retained)
      continue;

    QVariantMap noteData;
    noteData.insert(QStringLiteral("note"), note);
    noteData.insert(QStringLiteral("velocity"), static_cast<int>(state.velocity));
    notes.append(noteData);
    publishedMonitorNoteGenerations_[note] = state.onGeneration;
  }

  QVariantList gains;
  gains.reserve(MorphOutputCount);
  bool needsGainFollowUp = false;

  for (int outputIndex = 0; outputIndex < MorphOutputCount; ++outputIndex)
  {
    const double currentGain = displayedMorphOutputGains_[outputIndex];
    const double publishedGain = std::max(
      currentGain,
      pendingPeakMorphOutputGains_[outputIndex]);

    gains.append(std::clamp(publishedGain, 0.0, 1.0));
    needsGainFollowUp = needsGainFollowUp
                     || publishedGain > currentGain + 0.0001;
    pendingPeakMorphOutputGains_[outputIndex] = 0.0;
  }

  monitorFeedbackDirty_ = needsGainFollowUp;
  monitorFeedbackSnapshotPending_ = true;
  pendingMonitorFeedbackSequence_ = ++monitorFeedbackSequence_;

  emit monitorFeedbackSnapshot(notes,
                               gains,
                               pendingMonitorFeedbackSequence_);
}

//-----------------------------------------------------------------
void MidiEngine::acknowledgeMonitorFeedback(quint64 sequence)
//-----------------------------------------------------------------
{
  if (!monitorFeedbackSnapshotPending_
      || sequence != pendingMonitorFeedbackSequence_)
    return;

  monitorFeedbackSnapshotPending_ = false;
  const qint64 nowMs = monitorFeedbackClock_.isValid()
                     ? monitorFeedbackClock_.elapsed()
                     : 0;

  for (int note = 0; note < static_cast<int>(monitorFeedbackNotes_.size()); ++note)
  {
    const quint64 publishedGeneration = publishedMonitorNoteGenerations_[note];
    if (publishedGeneration == 0)
      continue;

    auto& state = monitorFeedbackNotes_[note];
    state.acknowledgedOnGeneration = std::max(
      state.acknowledgedOnGeneration,
      publishedGeneration);

    if (!state.active && state.onGeneration == publishedGeneration)
      state.retainUntilMs = std::max(
        state.retainUntilMs,
        nowMs + MonitorFeedbackMinVisibleMs);
  }
}

//------------------------------------------------------------------------------
bool MidiEngine::sendTrackNoteOn(int trackIndex, uint8_t note, uint8_t velocity)
//------------------------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return false;

  if (!openCurrentMidiOut(false))
    return false;

  const auto msg = MidiMessage::NoteOn(static_cast<uint8_t>(trackIndex), note, velocity);

  return midiOut_->sendShort(msg.status, msg.data1, msg.data2);
}

//-------------------------------------------------------------------------------
bool MidiEngine::sendTrackNoteOff(int trackIndex, uint8_t note, uint8_t velocity)
//-------------------------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return false;

  if (!openCurrentMidiOut(false))
    return false;

  const auto msg = MidiMessage::NoteOff(static_cast<uint8_t>(trackIndex), note, velocity);

  return midiOut_->sendShort(msg.status, msg.data1, msg.data2);
}

//------------------------------------------------------------------------
void MidiEngine::sendTrackControlChange(int trackIndex, int cc, int value)
//------------------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return;

  if (!openCurrentMidiOut(false))
    return;

  const auto msg = MidiMessage::CC(static_cast<uint8_t>(trackIndex), midi7bit(cc), midi7bit(value));

  midiOut_->sendShort(msg.status, msg.data1, msg.data2);
}

//------------------------------------------------------------------
void MidiEngine::sendTrackProgramChange(int trackIndex, int program)
//------------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return;

  if (!openCurrentMidiOut(false))
    return;

  const uint8_t channel = static_cast<uint8_t>(trackIndex);
  const uint8_t status = static_cast<uint8_t>(0xC0 | channel);

  midiOut_->sendShort(status, midi7bit(program), 0);
}

//---------------------------------------------------------------------------------------------------
void MidiEngine::sendTrackBankSelectAndProgram(int trackIndex, int bankMSB, int bankLSB, int program)
//---------------------------------------------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return;

  sendTrackControlChange(trackIndex, 0, bankMSB);
  sendTrackControlChange(trackIndex, 32, bankLSB);
  sendTrackProgramChange(trackIndex, program);
}

//---------------------------------------------------
void MidiEngine::sendCurrentPresetTrackSetup()
//---------------------------------------------------
{
  if (!openCurrentMidiOut(false))
    return;

  for (int trackIndex = 0; trackIndex < 16; ++trackIndex)
  {
    const auto& track = snapshot_.tracks[trackIndex];

    if (track.morphOutput == MorphOutputId::None)
      continue;

    /*
     * Program Change can reset controller and tuning state on the receiving
     * instrument, so send the patch first and then rebuild the complete
     * per-track setup.
     */
    sendTrackBankSelectAndProgram(trackIndex,
                                  track.program_cc0,
                                  track.program_cc32,
                                  track.program_number);

    sendTrackControlChange(trackIndex, 7, track.volume);
    sendTrackControlChange(trackIndex, 10,
                           std::clamp(static_cast<int>(track.panorama) + 64,
                                      0, 127));
    sendTrackControlChange(trackIndex, 91, track.reverb);
    sendTrackControlChange(trackIndex, 93, track.chorus);

    /* Start every newly loaded preset from full expression. */
    sendTrackControlChange(trackIndex, 11, 127);

    const uint8_t cc71 = timbreControllerValue(track.timbre1, 127);
    const uint8_t cc74 = timbreControllerValue(track.timbre2, 127);

    last_cc71_[trackIndex] = cc71;
    last_cc74_[trackIndex] = cc74;

    sendTrackControlChange(trackIndex, 71, cc71);
    sendTrackControlChange(trackIndex, 74, cc74);

    /* Program changes may also reset channel-wide RPN settings. */
    lastSentFineTuningCents_[trackIndex] = 1000;
    applyTrackTuning(trackIndex);
    sendPitchBendRange(trackIndex, snapshot_.pitchBendRange);
  }
}

//-----------------------------------------------------------
void MidiEngine::sendRNPFineTuning(int trackIndex, int cents)
//-----------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= 16)
    return;

  cents = std::clamp(cents, -100, 100);

  if (lastSentFineTuningCents_[trackIndex] == cents)
    return;

  /*
   * Channel Fine Tuning is RPN 0,1 and uses a 14-bit Data Entry value:
   *
   *   0x0000 = -100 cents
   *   0x4000 =    0 cents
   *   0x7FFF = almost +100 cents
   *
   * The resolution is 100 / 8192 cents per step.
   */
  const double clampedCents = static_cast<double>(cents);

  const int value14 = std::clamp(
    static_cast<int>(std::lround(
      8192.0 + clampedCents * 8192.0 / 100.0)),
    0,
    16383);

  const int dataMsb = (value14 >> 7) & 0x7f;
  const int dataLsb = value14 & 0x7f;

  // Select Channel Fine Tuning (RPN 0,1).
  sendTrackControlChange(trackIndex, 101, 0);
  sendTrackControlChange(trackIndex, 100, 1);

  // Send the complete 14-bit Data Entry value.
  sendTrackControlChange(trackIndex, 6,  dataMsb);
  sendTrackControlChange(trackIndex, 38, dataLsb);

  // Deselect the RPN to protect it from later Data Entry messages.
  sendTrackControlChange(trackIndex, 101, 127);
  sendTrackControlChange(trackIndex, 100, 127);

  lastSentFineTuningCents_[trackIndex] = cents;
}

//-----------------------------
void MidiEngine::sendGM2Reset()
//-----------------------------
{
  if (!openCurrentMidiOut(false))
    return;

  static constexpr uint8_t gm2Reset[] =
  {
    0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7
  };

  std::vector<uint8_t> msg(std::begin(gm2Reset), std::end(gm2Reset));

  midiOut_->sendSysEx(msg);

  /* GM2 Reset restores registered parameters, including channel tuning. */
  resetPerformanceState();
  lastSentFineTuningCents_.fill(1000);
  applyBaseTrackTunings(true);
  applyPitchBendRangeToAssignedTracks(true);
}

//-----------------------------------------------
void MidiEngine::sendSoftReset(int channel1Based)
//-----------------------------------------------
{
  if (!openCurrentMidiOut(false))
    return;

  const int ch1 = std::clamp(channel1Based, 1, 16);
  const uint8_t ch = static_cast<uint8_t>(ch1 - 1);

  // Release sustained notes before resetting the channel.
  sendTrackControlChange(ch, 64, 0);

  // All Notes Off
  sendTrackControlChange(ch, 123, 0);

  // Reset All Controllers
  sendTrackControlChange(ch, 121, 0);

  lastSentFineTuningCents_[ch] = 1000;
  applyTrackTuning(ch);
  sendPitchBendRange(ch, snapshot_.pitchBendRange);
}

//----------------------------------------------------------------
void MidiEngine::sendAllNotesOffAndExpressionReset(int trackIndex)
//----------------------------------------------------------------
{
  if (!openCurrentMidiOut(false))
    return;

  sendTrackControlChange(trackIndex, 64, 0);
  sendTrackControlChange(trackIndex, 123, 0);
  sendTrackControlChange(trackIndex, 11, 127);
}

//--------------------------------------------------
void MidiEngine::sendAllNotesOffAndExpressionReset()
//--------------------------------------------------
{
  if (openCurrentMidiOut(false))
  {
    for (int ch = 0; ch < 16; ++ch)
    {
      if (snapshot_.tracks[ch].morphOutput == MorphOutputId::None)
        continue;

      sendTrackControlChange(ch, 64, 0);
      sendTrackControlChange(ch, 123, 0);
      sendTrackControlChange(ch, 11, 127);
    }
  }

  resetPerformanceState();
}

//----------------------------------
bool MidiEngine::hasHeldNotes() const
//----------------------------------
{
  return surfaceTestNoteActive_
    || std::any_of(
      activeInputNotes_.begin(),
      activeInputNotes_.end(),
      [](bool active) { return active; })
    || std::any_of(
      activeTestNotes_.begin(),
      activeTestNotes_.end(),
      [](bool active) { return active; });
}

//--------------------------------------
bool MidiEngine::hasSoundingNotes() const
//--------------------------------------
{
  return hasHeldNotes()
    || (sustainDown_ && phraseActive_);
}

//-------------------------------
void MidiEngine::onMidiWatchdog()
//-------------------------------
{
#ifdef Q_OS_ANDROID
  constexpr qint64 idleMsBeforeReopen = 60000;

  const bool activeNotes = hasSoundingNotes();
  const bool midiRecentlyActive = lastMidiInActivity_.isValid() && lastMidiInActivity_.elapsed() < idleMsBeforeReopen;

  if (activeNotes && midiRecentlyActive)
    return;

  if (activeNotes && !midiRecentlyActive)
  {
    sendAllNotesOffAndExpressionReset();
    surfaceTestNoteActive_ = false;
  }

  refreshMidiPorts();
#endif
}

//------------------------------------------
bool MidiEngine::hasAnyAssignedTrack() const
//------------------------------------------
{
  for (const auto& track : snapshot_.tracks)
    if (track.morphOutput != MorphOutputId::None)
      return true;

  return false;
}


bool MidiEngine::isMorphOutputEnabled(MorphOutputId output) const
{
  const int outputIndex = static_cast<int>(output);
  if (outputIndex < 0 || outputIndex >= MorphOutputCount)
    return false;

  const uint8_t bit = static_cast<uint8_t>(1u << outputIndex);
  if ((snapshot_.morphOutputMuteMask & bit) != 0)
    return false;

  return snapshot_.morphOutputSoloMask == 0
      || (snapshot_.morphOutputSoloMask & bit) != 0;
}

//--------------------------------------------------------
uint8_t MidiEngine::noteFromNormalized(double xNorm) const
//--------------------------------------------------------
{
  xNorm = std::clamp(xNorm, 0.0, 1.0);

  const int note = int(std::lround(double(snapshot_.surfaceMinNote) + xNorm * double(snapshot_.surfaceMaxNote - snapshot_.surfaceMinNote)));

  return static_cast<uint8_t>(std::clamp(note, snapshot_.surfaceMinNote, snapshot_.surfaceMaxNote));
}

//------------------------------------------------------------
uint8_t MidiEngine::velocityFromNormalized(double yNorm) const
//------------------------------------------------------------
{
  yNorm = std::clamp(yNorm, 0.0, 1.0);

  const int velocity = 1 + int(std::lround(yNorm * 126.0));

  return static_cast<uint8_t>(std::clamp(velocity, 1, 127));
}

//---------------------------------------------------
void MidiEngine::testNoteOn(int note, int velocity)
//---------------------------------------------------
{
  note = std::clamp(note, 0, 127);
  velocity = std::clamp(velocity, 1, 127);

  const auto midiNote = static_cast<uint8_t>(note);
  const auto midiVelocity = static_cast<uint8_t>(velocity);

  if (activeTestNotes_[static_cast<size_t>(note)])
    return;

  activeTestNotes_[static_cast<size_t>(note)] = true;

  if (snapshot_.playMode == PlayMode::Poly)
  {
    prepareDetuneForNote(midiVelocity);
    handleIncomingNoteOn(midiNote, midiVelocity);
    return;
  }

  const bool hadSoundingNote = !currNotes_.empty();
  const uint8_t previousSoundingNote =
    hadSoundingNote ? currNotes_.back() : 0;

  currNotes_.remove(midiNote);

  if (hadSoundingNote && previousSoundingNote != midiNote)
    handleIncomingNoteOff(previousSoundingNote, midiVelocity);

  prepareDetuneForNote(midiVelocity);
  handleIncomingNoteOn(midiNote, midiVelocity);
  currNotes_.push_back(midiNote);
}

//-----------------------------------------
void MidiEngine::testNoteOff(int note)
//-----------------------------------------
{
  note = std::clamp(note, 0, 127);

  const auto midiNote = static_cast<uint8_t>(note);

  if (!activeTestNotes_[static_cast<size_t>(note)])
    return;

  activeTestNotes_[static_cast<size_t>(note)] = false;

  if (snapshot_.playMode == PlayMode::Poly)
  {
    handleIncomingNoteOff(midiNote, 64);
    updatePhraseStateAfterNoteOff();
    return;
  }

  if (!currNotes_.empty() && midiNote == currNotes_.back())
  {
    handleIncomingNoteOff(midiNote, 64);
    currNotes_.remove(midiNote);

    if (!currNotes_.empty())
    {
      prepareDetuneForNote(64);
      handleIncomingNoteOn(currNotes_.back(), 64);
    }
  }
  else
  {
    currNotes_.remove(midiNote);
  }

  updatePhraseStateAfterNoteOff();
}

//-------------------------------------------------------
void MidiEngine::surfacePress(double xNorm, double yNorm)
//-------------------------------------------------------
{
  const uint8_t note = noteFromNormalized(xNorm);
  const uint8_t velocity = velocityFromNormalized(yNorm);

  surfaceTestNoteActive_ = true;
  surfaceTestNote_ = note;
  surfaceTestVelocity_ = velocity;

  prepareDetuneForNote(velocity);
  handleIncomingNoteOn(note, velocity);
}

//------------------------------------------------------
void MidiEngine::surfaceMove(double xNorm, double yNorm)
//------------------------------------------------------
{
  if (!surfaceTestNoteActive_)
  {
    surfacePress(xNorm, yNorm);
    return;
  }

  const uint8_t note = noteFromNormalized(xNorm);
  const uint8_t velocity = velocityFromNormalized(yNorm);

  if (note == surfaceTestNote_ && velocity == surfaceTestVelocity_)
    return;

  handleIncomingNoteOff(surfaceTestNote_, 100);

  surfaceTestNote_ = note;
  surfaceTestVelocity_ = velocity;

  prepareDetuneForNote(surfaceTestVelocity_);
  handleIncomingNoteOn(surfaceTestNote_, surfaceTestVelocity_);
}

//-------------------------------
void MidiEngine::surfaceRelease()
//-------------------------------
{
  if (!surfaceTestNoteActive_)
    return;

  handleIncomingNoteOff(surfaceTestNote_, 100);

  surfaceTestNoteActive_ = false;
  surfaceTestNote_ = 0;
  surfaceTestVelocity_ = 0;

  updatePhraseStateAfterNoteOff();
}
