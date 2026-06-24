#pragma once

#include <QString>
#include <QStringList>
#include <QObject>
#include <QVector>
#include <array>
#include <cstdint>

#include "TrackGroupId.h"
#include "Footage.h"
#include "KeyboardStandardRanges.h"
#include "PiecewiseCurve.h"
#include "InstrumentDefinition.h"


struct TrackPresetData
{
  bool valid = false; // true se la traccia è assegnata a un gruppo

  TrackGroupId group = TrackGroupId::None;

  Footage footage = Footage::ftg8;
  uint8_t program_cc0;
  uint8_t program_cc32;
  uint8_t program_number;
  int8_t  panorama = 0;
  uint8_t volume = 100;
  int8_t  reverb = 0;
  int8_t  chorus = 0;
  int8_t  timbre1 = 0;
  int8_t  timbre2 = 0;

  uint8_t curve1 = 0;
  uint8_t curve2 = 0;
};

enum class PlayMode { Poly, MonoRetrigVelOff, MonoRetrigOrigVel, MonoNoRetrig };

struct MidiSetupData
{
  QString midiInPort;
  QString midiOutPort;
  uint8_t midiInChannel = 0;
};

struct AppInitSettings
{
  MidiSetupData midiSetup;
  PlayMode playMode = PlayMode::MonoRetrigVelOff;
  PatchPolicy patchPolicy = PatchPolicy::Manual;
  QString knownInstrumentName;
  KeyboardRangeId keyboardRangeId = KeyboardRangeId::Full;
};

struct PresetData
{
  QString name;

  AppInitSettings appInitSettings;
  std::array<TrackGroupId, 16> assignments;
  std::array<TrackPresetData, 16> tracks;
  std::array<PiecewiseCurve, numOfCurves> keyCurves;
  std::array<PiecewiseCurve, numOfCurves> velCurves;
};

//----------------------------------
class PresetManager : public QObject
//----------------------------------
{
  Q_OBJECT

public:
  explicit PresetManager(QObject* parent = nullptr);

  const QVector<PresetData>& presets() const;
  QStringList presetNames() const;

  bool loadFromDisk();
  bool saveToDisk() const;

  bool savePreset(const PresetData& preset);   // crea o sovrascrive per nome
  bool removePreset(const QString& name);
  const PresetData* findPreset(const QString& name) const;
  bool contains(const QString& key) const;

signals:
  void presetsChanged();

private:
  QVector<PresetData> presets_;
};