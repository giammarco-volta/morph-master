#include <algorithm>
#include <vector>

#include "TrackController.h"
#include "SettingsController.h"

namespace
{
  constexpr uint8_t CC_VOLUME = 7;
  constexpr uint8_t CC_PAN = 10;
  constexpr uint8_t CC_REVERB = 91;
  constexpr uint8_t CC_CHORUS = 93;
  constexpr uint8_t CC_TIMBRE_1 = 71;
  constexpr uint8_t CC_TIMBRE_2 = 74;

  //------------------------------------
  uint8_t centeredToMidiValue(int value)
  //------------------------------------
  {
    return static_cast<uint8_t>(std::clamp(value + 64, 0, 127));
  }

  //-------------------------
  uint8_t midi7bit(int value)
  //-------------------------
  {
    return static_cast<uint8_t>(std::clamp(value, 0, 127));
  }

  //-----------------------------------------------------
  QString programDisplayName(const ProgramEntry& program)
  //-----------------------------------------------------
  {
    return QString::fromStdString(program.name).trimmed();
  }
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TrackController::TrackController(SettingsController& settingsController, int trackIndex, QObject* parent) : QObject(parent), settingsController_(settingsController), trackIndex_(trackIndex)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
  connect(&settingsController_,
    &SettingsController::knownInstrumentNameChanged,
    this,
    [this]()
    {
      emit instrumentProgramsChanged();
      emit instrumentProgramDisplayNameChanged();
    });
}

//--------------------------------
int TrackController::morphOutput() const
//--------------------------------
{
  const auto& preset = settingsController_.currentPreset();

  return static_cast<int>(preset.tracks[trackIndex_].morphOutput);
}

//---------------------------------------
void TrackController::setMorphOutput(int value)
//---------------------------------------
{
  auto& preset = settingsController_.currentPreset();
  auto& track = preset.tracks[trackIndex_];

  const int clampedValue = std::clamp(
    value,
    static_cast<int>(MorphOutputId::Loud),
    static_cast<int>(MorphOutputId::None));

  const auto newMorphOut = static_cast<MorphOutputId>(clampedValue);
  const auto profile = MorphOutputProfile::GetProfile(newMorphOut);

  const bool morphOutChangedValue = track.morphOutput != newMorphOut;
  bool targetHadTracks = false;
  if (newMorphOut != MorphOutputId::None)
  {
    for (int i = 0; i < static_cast<int>(preset.tracks.size()); ++i)
    {
      if (i != trackIndex_ && preset.tracks[i].morphOutput == newMorphOut)
      {
        targetHadTracks = true;
        break;
      }
    }
  }

  track.morphOutput = newMorphOut;

  if (morphOutChangedValue && newMorphOut != MorphOutputId::None && !targetHadTracks)
    settingsController_.initializeMorphOutputNameIfNeeded(clampedValue, trackIndex_);

  if (morphOutChangedValue)
    emit morphOutputChanged();

  settingsController_.notifySurfaceMorphOutputsChanged(trackIndex_);
  settingsController_.notifyMidiRelevantStateChanged();
}

//--------------------------------
int TrackController::footage() const
//--------------------------------
{
  const auto& preset = settingsController_.currentPreset();

  return static_cast<int>(preset.tracks[trackIndex_].footage);
}

//-----------------------------------------
void TrackController::setFootage(int value)
//-----------------------------------------
{
  auto& preset = settingsController_.currentPreset();

  const auto newFootage = static_cast<Footage>(value);

  if (preset.tracks[trackIndex_].footage == newFootage)
    return;

  preset.tracks[trackIndex_].footage = newFootage;

  emit footageChanged();

  settingsController_.notifyMidiRelevantStateChanged();
}

//-------------------------------------
int TrackController::detuneOffset() const
//-------------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].detuneOffset;
}

//--------------------------------------------
void TrackController::setDetuneOffset(int value)
//--------------------------------------------
{
  auto& track = settingsController_.currentPreset().tracks[trackIndex_];
  const auto newValue = static_cast<int8_t>(std::clamp(value, -50, 50));

  if (track.detuneOffset == newValue)
    return;

  track.detuneOffset = newValue;

  emit detuneOffsetChanged();
  settingsController_.notifyMidiRelevantStateChanged();
}

//-------------------------------------
int TrackController::detuneSpread() const
//-------------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].detuneSpread;
}

//--------------------------------------------
void TrackController::setDetuneSpread(int value)
//--------------------------------------------
{
  auto& track = settingsController_.currentPreset().tracks[trackIndex_];
  const auto newValue = static_cast<uint8_t>(std::clamp(value, 0, 50));

  if (track.detuneSpread == newValue)
    return;

  track.detuneSpread = newValue;

  emit detuneSpreadChanged();
  settingsController_.notifyMidiRelevantStateChanged();
}

//---------------------------------
int TrackController::volume() const
//---------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].volume;
}

//----------------------------------------
void TrackController::setVolume(int value)
//----------------------------------------
{
  auto& preset = settingsController_.currentPreset();
  auto& track = preset.tracks[trackIndex_];

  const auto newVolume = static_cast<uint8_t>(std::clamp(value, 0, 127));

  if (track.volume == newVolume)
    return;

  track.volume = newVolume;

  settingsController_.sendTrackControlChange(trackIndex_, CC_VOLUME, newVolume);

  emit volumeChanged();
}

//------------------------------
int TrackController::pan() const
//------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].panorama;
}

//-------------------------------------
void TrackController::setPan(int value)
//------------------------------------
{
  auto& preset = settingsController_.currentPreset();
  auto& track = preset.tracks[trackIndex_];

  const auto newPan = static_cast<int8_t>(std::clamp(value + 64, 0, 127) - 64);

  if (track.panorama == newPan)
    return;

  track.panorama = newPan;

  settingsController_.sendTrackControlChange(trackIndex_, CC_PAN, centeredToMidiValue(newPan));

  emit panChanged();
}

//---------------------------------
int TrackController::reverb() const
//---------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].reverb;
}

//----------------------------------------
void TrackController::setReverb(int value)
//----------------------------------------
{
  auto& preset = settingsController_.currentPreset();
  auto& track = preset.tracks[trackIndex_];

  const auto newReverb = static_cast<uint8_t>(std::clamp(value, 0, 127));

  if (track.reverb == newReverb)
    return;

  track.reverb = newReverb;

  settingsController_.sendTrackControlChange(trackIndex_, CC_REVERB, static_cast<uint8_t>(newReverb));

  emit reverbChanged();
}

//---------------------------------
int TrackController::chorus() const
//---------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].chorus;
}

//----------------------------------------
void TrackController::setChorus(int value)
//----------------------------------------
{
  auto& preset = settingsController_.currentPreset();
  auto& track = preset.tracks[trackIndex_];

  const auto newChorus = static_cast<uint8_t>(std::clamp(value, 0, 127));

  if (track.chorus == newChorus)
    return;

  track.chorus = newChorus;

  settingsController_.sendTrackControlChange(trackIndex_, CC_CHORUS, static_cast<uint8_t>(newChorus));

  emit chorusChanged();
}

//---------------------------------
int TrackController::tone() const
//---------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].timbre1;
}

//----------------------------------------
void TrackController::setTone(int value)
//----------------------------------------
{
  auto& preset = settingsController_.currentPreset();
  auto& track = preset.tracks[trackIndex_];

  cc71contribute_ = static_cast<int8_t>(std::clamp(value + 64, 0, 127) - 64);

  if (track.timbre1 == cc71contribute_)
    return;

  track.timbre1 = cc71contribute_;

  emit toneChanged();

  settingsController_.notifyMidiRelevantStateChanged();
}

//---------------------------------
int TrackController::timbre() const
//---------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].timbre2;
}

//----------------------------------------
void TrackController::setTimbre(int value)
//----------------------------------------
{
  auto& preset = settingsController_.currentPreset();
  auto& track = preset.tracks[trackIndex_];

  cc74contribute_ = static_cast<int8_t>(std::clamp(value + 64, 0, 127) - 64);

  if (track.timbre2 == cc74contribute_)
    return;

  track.timbre2 = cc74contribute_;

  emit timbreChanged();

  settingsController_.notifyMidiRelevantStateChanged();
}

//---------------------------------------------
int TrackController::instrumentProgramCount() const
//---------------------------------------------
{
  const InstrumentDefinition* def = settingsController_.currentInstrumentDefinition();
  if (!def)
    return 0;

  return static_cast<int>(std::count_if(def->programs.cbegin(), def->programs.cend(),
    [](const ProgramEntry& program)
    {
      return program.msb >= 0 && program.lsb >= 0 && program.program >= 0;
    }));
}

//-------------------------------------------------------------------------------------------------------
QVariantList TrackController::findInstrumentPrograms(const QString& nameFilter, int programNumber) const
//-------------------------------------------------------------------------------------------------------
{
  QVariantList result;
  const InstrumentDefinition* def = settingsController_.currentInstrumentDefinition();
  if (!def)
    return result;

  const QString normalizedFilter = nameFilter.simplified().toLower();
  const QStringList tokens = normalizedFilter.split(' ', Qt::SkipEmptyParts);
  const bool useTextFilter = normalizedFilter.size() >= 2;
  const bool useProgramFilter = programNumber >= 0 && programNumber <= 127;

  result.reserve(static_cast<qsizetype>(def->programs.size()));
  for (const ProgramEntry& program : def->programs)
  {
    if (program.msb < 0 || program.lsb < 0 || program.program < 0)
      continue;

    if (useProgramFilter && program.program != programNumber)
      continue;

    const QString displayName = programDisplayName(program);
    if (useTextFilter)
    {
      const QString normalizedName = displayName.toLower();
      bool matchesAllTokens = true;
      for (const QString& token : tokens)
      {
        if (!normalizedName.contains(token))
        {
          matchesAllTokens = false;
          break;
        }
      }
      if (!matchesAllTokens)
        continue;
    }

    QVariantMap item;
    item.insert("programName", displayName);
    item.insert("bankName", QString::fromStdString(program.bankName).trimmed());
    item.insert("msb", program.msb);
    item.insert("lsb", program.lsb);
    item.insert("programNumber", program.program);
    result.push_back(std::move(item));
  }

  return result;
}

//------------------------------------------------------
QString TrackController::instrumentProgramDisplayName() const
//------------------------------------------------------
{
  const InstrumentDefinition* def = settingsController_.currentInstrumentDefinition();
  const auto& track = settingsController_.currentPreset().tracks[trackIndex_];
  if (def)
  {
    for (const ProgramEntry& program : def->programs)
    {
      if (program.msb == track.program_cc0 && program.lsb == track.program_cc32 && program.program == track.program_number)
        return programDisplayName(program);
    }
  }
  return QString("MSB %1 · LSB %2 · Program %3").arg(track.program_cc0).arg(track.program_cc32).arg(track.program_number + 1);
}

//--------------------------------------------------------------------------
void TrackController::selectInstrumentProgram(int bankMSB, int bankLSB, int programNumber)
//--------------------------------------------------------------------------
{
  auto& track = settingsController_.currentPreset().tracks[trackIndex_];
  const auto newCc0 = static_cast<uint8_t>(std::clamp(bankMSB, 0, 127));
  const auto newCc32 = static_cast<uint8_t>(std::clamp(bankLSB, 0, 127));
  const auto newProgram = static_cast<uint8_t>(std::clamp(programNumber, 0, 127));
  const bool cc0Changed = track.program_cc0 != newCc0;
  const bool cc32Changed = track.program_cc32 != newCc32;
  const bool programChanged = track.program_number != newProgram;
  if (!cc0Changed && !cc32Changed && !programChanged)
    return;
  track.program_cc0 = newCc0;
  track.program_cc32 = newCc32;
  track.program_number = newProgram;
  settingsController_.sendTrackBankSelectAndProgram(trackIndex_, track.program_cc0, track.program_cc32, track.program_number);
  emit instrumentProgramDisplayNameChanged();
  if (cc0Changed) emit bankMSBChanged();
  if (cc32Changed) emit bankLSBChanged();
  if (programChanged) emit programNumberChanged();
}

//----------------------------------
int TrackController::bankMSB() const
//----------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].program_cc0;
}

//-----------------------------------------
void TrackController::setBankMSB(int value)
//-----------------------------------------
{
  auto& track = settingsController_.currentPreset().tracks[trackIndex_];

  const auto newValue = static_cast<uint8_t>(std::clamp(value, 0, 127));

  if (track.program_cc0 == newValue)
    return;

  track.program_cc0 = newValue;

  emit bankMSBChanged();
  emit instrumentProgramDisplayNameChanged();
}

//----------------------------------
int TrackController::bankLSB() const
//----------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].program_cc32;
}

//-----------------------------------------
void TrackController::setBankLSB(int value)
//-----------------------------------------
{
  auto& track = settingsController_.currentPreset().tracks[trackIndex_];

  const auto newValue = static_cast<uint8_t>(std::clamp(value, 0, 127));

  if (track.program_cc32 == newValue)
    return;

  track.program_cc32 = newValue;

  emit bankLSBChanged();
  emit instrumentProgramDisplayNameChanged();
}

//----------------------------------------
int TrackController::programNumber() const
//----------------------------------------
{
  const auto& preset = settingsController_.currentPreset();
  return preset.tracks[trackIndex_].program_number;
}

//-----------------------------------------------
void TrackController::setProgramNumber(int value)
//-----------------------------------------------
{
  auto& track = settingsController_.currentPreset().tracks[trackIndex_];

  const auto newValue = static_cast<uint8_t>(std::clamp(value, 0, 127));

  if (track.program_number == newValue)
    return;

  track.program_number = newValue;

  settingsController_.sendTrackBankSelectAndProgram(trackIndex_, track.program_cc0, track.program_cc32, track.program_number);

  emit programNumberChanged();
  emit instrumentProgramDisplayNameChanged();
}

//---------------------------------------
void TrackController::notifyDataChanged()
//---------------------------------------
{
  emit morphOutputChanged();

  emit footageChanged();
  emit detuneOffsetChanged();
  emit detuneSpreadChanged();

  emit instrumentProgramsChanged();
  emit instrumentProgramDisplayNameChanged();

  emit bankMSBChanged();
  emit bankLSBChanged();
  emit programNumberChanged();

  emit volumeChanged();
  emit panChanged();
  emit reverbChanged();
  emit chorusChanged();
  emit toneChanged();
  emit timbreChanged();
}