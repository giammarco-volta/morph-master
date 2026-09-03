#include "Presets.h"

#include <algorithm>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

// ============================================================
// Helpers JSON
// ============================================================

namespace
{
  static constexpr int kPresetFileVersion = 1;
  static constexpr int kNumTracks = 16;

  QString presetsFilePath()
  {
    QString baseDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (baseDir.isEmpty())
      baseDir = QDir::homePath() + "/.MorphMaster";

    QDir dir(baseDir);
    if (!dir.exists())
      dir.mkpath(".");

    return dir.filePath("presets.json");
  }

  // ------------------------------
  // enum <-> int helpers
  // ------------------------------
  template <typename EnumT>
  int enumToInt(EnumT value)
  {
    return static_cast<int>(value);
  }

  template <typename EnumT>
  EnumT intToEnum(int value, EnumT defaultValue)
  {
    return static_cast<EnumT>(value);
  }

  // ------------------------------
  // TrackPresetData
  // ------------------------------
  QJsonObject toJson(const TrackPresetData& t)
  {
    QJsonObject obj;

    obj["morphOutput"] = enumToInt(t.morphOutput);
    obj["footage"] = enumToInt(t.footage);

    obj["program_cc0"] = static_cast<int>(t.program_cc0);
    obj["program_cc32"] = static_cast<int>(t.program_cc32);
    obj["program_number"] = static_cast<int>(t.program_number);

    obj["panorama"] = static_cast<int>(t.panorama);
    obj["volume"] = static_cast<int>(t.volume);
    obj["reverb"] = static_cast<int>(t.reverb);
    obj["chorus"] = static_cast<int>(t.chorus);
    obj["timbre1"] = static_cast<int>(t.timbre1);
    obj["timbre2"] = static_cast<int>(t.timbre2);
    obj["detuneOffset"] = static_cast<int>(t.detuneOffset);
    obj["detuneSpread"] = static_cast<int>(t.detuneSpread);

    return obj;
  }

  TrackPresetData trackPresetDataFromJson(const QJsonObject& obj)
  {
    TrackPresetData t;

    const int morphOutputValue = obj.contains("morphOutput")
      ? obj.value("morphOutput").toInt(enumToInt(MorphOutputId::None))
      : obj.value("group").toInt(enumToInt(MorphOutputId::None));

    t.morphOutput = intToEnum<MorphOutputId>(morphOutputValue,
                                             MorphOutputId::None);
    t.footage = intToEnum<Footage>(obj.value("footage").toInt(enumToInt(Footage::ftg8)), Footage::ftg8);

    t.program_cc0 = static_cast<uint8_t>(obj.value("program_cc0").toInt(0));
    t.program_cc32 = static_cast<uint8_t>(obj.value("program_cc32").toInt(0));
    t.program_number = static_cast<uint8_t>(obj.value("program_number").toInt(0));

    t.panorama = static_cast<int8_t>(obj.value("panorama").toInt(0));
    t.volume = static_cast<uint8_t>(obj.value("volume").toInt(100));
    t.reverb = static_cast<uint8_t>(obj.value("reverb").toInt(40));
    t.chorus = static_cast<uint8_t>(obj.value("chorus").toInt(0));
    t.timbre1 = static_cast<int8_t>(obj.value("timbre1").toInt(0));
    t.timbre2 = static_cast<int8_t>(obj.value("timbre2").toInt(0));
    t.detuneOffset = static_cast<int8_t>(std::clamp(
      obj.value("detuneOffset").toInt(0), -50, 50));
    t.detuneSpread = static_cast<uint8_t>(std::clamp(
      obj.value("detuneSpread").toInt(0), 0, 50));

    return t;
  }


  QJsonObject toJson(const MorphOutputPresetData& output)
  {
    QJsonObject obj;
    obj["name"] = output.name;
    obj["customName"] = output.customName;
    return obj;
  }

  MorphOutputPresetData morphOutputPresetDataFromJson(const QJsonObject& obj)
  {
    MorphOutputPresetData output;
    output.name = obj.value("name").toString().trimmed().left(40);
    output.customName = obj.value("customName").toBool(false);
    return output;
  }

  // ------------------------------
  // PiecewiseCurve
  // ------------------------------
  QJsonObject toJson(const PiecewiseCurve& c)
  {
    QJsonObject obj;
    obj["x0"] = static_cast<int>(c.x0);
    obj["y0"] = static_cast<int>(c.y0);
    obj["x1"] = static_cast<int>(c.x1);
    obj["y1"] = static_cast<int>(c.y1);
    return obj;
  }

  PiecewiseCurve piecewiseCurveFromJson(const QJsonObject& obj)
  {
    PiecewiseCurve c;
    c.x0 = static_cast<uint8_t>(obj.value("x0").toInt(c.x0));
    c.y0 = static_cast<uint8_t>(obj.value("y0").toInt(c.y0));
    c.x1 = static_cast<uint8_t>(obj.value("x1").toInt(c.x1));
    c.y1 = static_cast<uint8_t>(obj.value("y1").toInt(c.y1));
    return c;
  }

  // ------------------------------
  // MidiSetupData
  // ------------------------------
  QJsonObject toJson(const MidiSetupData& m)
  {
    QJsonObject obj;
    obj["midiInPort"] = m.midiInPort;
    obj["midiOutPort"] = m.midiOutPort;
    obj["midiInChannel"] = static_cast<int>(m.midiInChannel);
    obj["filterPresetControlChanges"] = m.filterPresetControlChanges;
    return obj;
  }

  MidiSetupData midiSetupDataFromJson(const QJsonObject& obj)
  {
    MidiSetupData m;
    m.midiInPort = obj.value("midiInPort").toString();
    m.midiOutPort = obj.value("midiOutPort").toString();
    m.midiInChannel = static_cast<uint8_t>(obj.value("midiInChannel").toInt(0));
    m.filterPresetControlChanges =
      obj.value("filterPresetControlChanges").toBool(true);
    return m;
  }

  // ------------------------------
  // PresetData
  // ------------------------------

  QJsonObject toJson(const AppInitSettings& a)
  {
    QJsonObject obj;

    obj["midiSetup"] = toJson(a.midiSetup);
    obj["playMode"] = enumToInt(a.playMode);
    obj["patchPolicy"] = enumToInt(a.patchPolicy);
    obj["knownInstrumentName"] = a.knownInstrumentName;
    obj["keyboardRangeId"] = enumToInt(a.keyboardRangeId);
    obj["pitchBendRange"] = static_cast<int>(a.pitchBendRange);
    obj["morphOutputMuteMask"] = static_cast<int>(a.morphOutputMuteMask);
    obj["morphOutputSoloMask"] = static_cast<int>(a.morphOutputSoloMask);

    return obj;
  }

  AppInitSettings appInitSettingsFromJson(const QJsonObject& obj)
  {
    AppInitSettings a;

    a.midiSetup = midiSetupDataFromJson(obj.value("midiSetup").toObject());
    a.playMode = intToEnum<PlayMode>(obj.value("playMode").toInt(enumToInt(a.playMode)), a.playMode);
    a.patchPolicy = intToEnum<PatchPolicy>(obj.value("patchPolicy").toInt(enumToInt(a.patchPolicy)), a.patchPolicy);
    a.knownInstrumentName = obj.value("knownInstrumentName").toString(a.knownInstrumentName);
    a.keyboardRangeId = intToEnum<KeyboardRangeId>(obj.value("keyboardRangeId").toInt(enumToInt(a.keyboardRangeId)), a.keyboardRangeId);
    a.pitchBendRange = static_cast<uint8_t>(std::clamp(obj.value("pitchBendRange").toInt(a.pitchBendRange), 0, 24));
    a.morphOutputMuteMask = static_cast<uint8_t>(std::clamp(obj.value("morphOutputMuteMask").toInt(a.morphOutputMuteMask), 0, 255));
    a.morphOutputSoloMask = static_cast<uint8_t>(std::clamp(obj.value("morphOutputSoloMask").toInt(a.morphOutputSoloMask), 0, 255));

    return a;
  }

  QJsonObject toJson(const PresetData& p)
  {
    QJsonObject obj;
    obj["name"] = p.name;
    obj["notes"] = p.notes;
    obj["appInitSettings"] = toJson(p.appInitSettings);

    QJsonArray tracksArray;
    for (const auto& t : p.tracks)
      tracksArray.append(toJson(t));
    obj["tracks"] = tracksArray;

    QJsonArray morphOutputsArray;
    for (const auto& output : p.morphOutputs)
      morphOutputsArray.append(toJson(output));
    obj["morphOutputs"] = morphOutputsArray;

    obj["keycurve"] = toJson(p.keyCurve);
    obj["velcurve"] = toJson(p.velCurve);

    return obj;
  }

  PresetData presetDataFromJson(const QJsonObject& obj)
  {
    PresetData p;
    p.name = obj.value("name").toString().trimmed();
    p.notes = obj.value("notes").toString();

    if (obj.contains("appInitSettings") && obj.value("appInitSettings").isObject())
    {
      p.appInitSettings = appInitSettingsFromJson(obj.value("appInitSettings").toObject());
    }

    {
      const QJsonArray arr = obj.value("tracks").toArray();
      for (int i = 0; i < kNumTracks; ++i)
      {
        if (i < arr.size() && arr.at(i).isObject())
          p.tracks[i] = trackPresetDataFromJson(arr.at(i).toObject());
        else
          p.tracks[i] = TrackPresetData{};
      }
    }

    {
      const QJsonArray arr = obj.value("morphOutputs").toArray();
      for (int i = 0; i < static_cast<int>(p.morphOutputs.size()); ++i)
      {
        if (i < arr.size() && arr.at(i).isObject())
          p.morphOutputs[i] = morphOutputPresetDataFromJson(arr.at(i).toObject());
      }
    }

    const auto readSingleCurve = [](const QJsonValue& value,
                                    PiecewiseCurve defaultCurve)
    {
      if (value.isObject())
        return piecewiseCurveFromJson(value.toObject());

      /* Compatibility with the short-lived array-based representation. */
      if (value.isArray())
      {
        const QJsonArray array = value.toArray();

        if (!array.isEmpty() && array.first().isObject())
          return piecewiseCurveFromJson(array.first().toObject());
      }

      return defaultCurve;
    };

    p.keyCurve = readSingleCurve(obj.value("keycurve"), p.keyCurve);
    p.velCurve = readSingleCurve(obj.value("velcurve"), p.velCurve);

    return p;
  }
}

// ============================================================
// PresetManager
// ============================================================

PresetManager::PresetManager(QObject* parent) : QObject(parent)
{
}

const QVector<PresetData>& PresetManager::presets() const
{
  return presets_;
}

QStringList PresetManager::presetNames() const
{
  QStringList names;
  names.reserve(presets_.size());

  for (const auto& p : presets_)
    names.append(p.name);

  return names;
}

bool PresetManager::loadFromDisk()
{
  const QString path = presetsFilePath();
  QFile file(path);

  if (!file.exists())
  {
    presets_.clear();
    emit presetsChanged();
    return true;
  }

  if (!file.open(QIODevice::ReadOnly))
    return false;

  const QByteArray data = file.readAll();
  file.close();

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    return false;

  const QJsonObject root = doc.object();
  const int version = root.value("version").toInt(0);
  if (version != kPresetFileVersion)
  {
    // Per ora accettiamo solo la versione corrente.
    return false;
  }

  const QJsonArray presetsArray = root.value("presets").toArray();

  QVector<PresetData> loaded;
  loaded.reserve(presetsArray.size());

  for (const auto& v : presetsArray)
  {
    if (!v.isObject())
      continue;

    PresetData p = presetDataFromJson(v.toObject());
    if (p.name.trimmed().isEmpty())
      continue;

    loaded.push_back(std::move(p));
  }

  presets_ = std::move(loaded);
  emit presetsChanged();
  return true;
}

bool PresetManager::saveToDisk() const
{
  const QString path = presetsFilePath();

  QJsonObject root;
  root["version"] = kPresetFileVersion;

  QJsonArray presetsArray;
  for (const auto& p : presets_)
    presetsArray.append(toJson(p));

  root["presets"] = presetsArray;

  const QJsonDocument doc(root);

  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    return false;

  const QByteArray json = doc.toJson(QJsonDocument::Indented);
  if (file.write(json) != json.size())
    return false;

  return file.commit();
}

bool PresetManager::savePreset(const PresetData& preset)
{
  const QString trimmedName = preset.name.trimmed();
  if (trimmedName.isEmpty())
    return false;

  PresetData normalized = preset;
  normalized.name = trimmedName;

  for (auto& existing : presets_)
  {
    if (existing.name == trimmedName)
    {
      existing = normalized;

      if (!saveToDisk())
        return false;

      emit presetsChanged();
      return true;
    }
  }

  presets_.push_back(normalized);

  if (!saveToDisk())
    return false;

  emit presetsChanged();
  return true;
}

bool PresetManager::removePreset(const QString& name)
{
  const QString trimmedName = name.trimmed();
  if (trimmedName.isEmpty())
    return false;

  for (auto it = presets_.begin(); it != presets_.end(); ++it)
  {
    if (it->name == trimmedName)
    {
      presets_.erase(it);

      if (!saveToDisk())
        return false;

      emit presetsChanged();
      return true;
    }
  }

  return false;
}

const PresetData* PresetManager::findPreset(const QString& name) const
{
  const QString trimmedName = name.trimmed();

  for (const auto& p : presets_)
  {
    if (p.name == trimmedName)
      return &p;
  }

  return nullptr;
}

bool PresetManager::contains(const QString& name) const
{
  for (const auto& p : presets_)
  {
    if (p.name.compare(name, Qt::CaseInsensitive) == 0)
      return true;
  }
  return false;
}