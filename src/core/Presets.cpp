#include "Presets.h"

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
    obj["valid"] = t.valid;
    obj["group"] = enumToInt(t.group);
    obj["footage"] = enumToInt(t.footage);
    obj["program_cc0"] = static_cast<int>(t.program_cc0);
    obj["program_cc32"] = static_cast<int>(t.program_cc32);
    obj["program_number"] = static_cast<int>(t.program_number);
    obj["panorama"] = static_cast<int>(t.panorama);
    obj["volume"] = static_cast<int>(t.volume);
    obj["timbre1"] = static_cast<int>(t.timbre1);
    obj["timbre2"] = static_cast<int>(t.timbre2);
    obj["curve1"] = static_cast<int>(t.curve1);
    obj["curve2"] = static_cast<int>(t.curve2);
    return obj;
  }

  TrackPresetData trackPresetDataFromJson(const QJsonObject& obj)
  {
    TrackPresetData t;

    t.valid = obj.value("valid").toBool(false);
    t.group = intToEnum<TrackGroupId>(
      obj.value("group").toInt(enumToInt(TrackGroupId::None)),
      TrackGroupId::None);
    t.footage = intToEnum<Footage>(
      obj.value("footage").toInt(enumToInt(Footage::ftg8)),
      Footage::ftg8);
    t.program_cc0 = static_cast<uint8_t>(obj.value("program_cc0").toInt(0));
    t.program_cc32 = static_cast<uint8_t>(obj.value("program_cc32").toInt(0));
    t.program_number = static_cast<uint8_t>(obj.value("program_number").toInt(0));
    t.panorama = static_cast<int8_t>(obj.value("panorama").toInt(0));
    t.volume = static_cast<uint8_t>(obj.value("volume").toInt(100));
    t.timbre1 = static_cast<int8_t>(obj.value("timbre1").toInt(0));
    t.timbre2 = static_cast<int8_t>(obj.value("timbre2").toInt(0));
    t.curve1 = static_cast<uint8_t>(obj.value("curve1").toInt(0));
    t.curve2 = static_cast<uint8_t>(obj.value("curve2").toInt(0));

    return t;
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
    return obj;
  }

  MidiSetupData midiSetupDataFromJson(const QJsonObject& obj)
  {
    MidiSetupData m;
    m.midiInPort = obj.value("midiInPort").toString();
    m.midiOutPort = obj.value("midiOutPort").toString();
    m.midiInChannel = static_cast<uint8_t>(obj.value("midiInChannel").toInt(0));
    return m;
  }

  // ------------------------------
  // PresetData
  // ------------------------------
  QJsonObject toJson(const PresetData& p)
  {
    QJsonObject obj;
    obj["name"] = p.name;
    obj["keyboardRangeId"] = enumToInt(p.appInitSettings.keyboardRangeId);
    obj["midiSetup"] = toJson(p.appInitSettings.midiSetup);

    QJsonArray assignmentsArray;
    for (const auto& a : p.assignments)
      assignmentsArray.append(enumToInt(a));
    obj["assignments"] = assignmentsArray;

    QJsonArray tracksArray;
    for (const auto& t : p.tracks)
      tracksArray.append(toJson(t));
    obj["tracks"] = tracksArray;

    QJsonArray keyCurvesArray;
    for (const auto& c : p.keyCurves)
      keyCurvesArray.append(toJson(c));
    obj["keycurves"] = keyCurvesArray;

    QJsonArray velCurvesArray;
    for (const auto& c : p.velCurves)
      velCurvesArray.append(toJson(c));
    obj["velcurves"] = velCurvesArray;

    return obj;
  }

  PresetData presetDataFromJson(const QJsonObject& obj)
  {
    PresetData p;
    p.name = obj.value("name").toString().trimmed();
    p.appInitSettings.keyboardRangeId = intToEnum<KeyboardRangeId>(
      obj.value("keyboardRangeId").toInt(enumToInt(KeyboardRangeId::Full)),
      KeyboardRangeId::Full);
    p.appInitSettings.midiSetup = midiSetupDataFromJson(obj.value("midiSetup").toObject());

    {
      const QJsonArray arr = obj.value("assignments").toArray();
      for (int i = 0; i < kNumTracks; ++i)
      {
        const int value = (i < arr.size())
          ? arr.at(i).toInt(enumToInt(TrackGroupId::None))
          : enumToInt(TrackGroupId::None);

        p.assignments[i] = intToEnum<TrackGroupId>(value, TrackGroupId::None);
      }
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
      const QJsonArray arr = obj.value("keycurves").toArray();
      for (int i = 0; i < static_cast<int>(numOfCurves); ++i)
      {
        if (i < arr.size() && arr.at(i).isObject())
          p.keyCurves[i] = piecewiseCurveFromJson(arr.at(i).toObject());
        else
          p.keyCurves[i] = PiecewiseCurve{};
      }
    }

    {
      const QJsonArray arr = obj.value("velcurves").toArray();
      for (int i = 0; i < static_cast<int>(numOfCurves); ++i)
      {
        if (i < arr.size() && arr.at(i).isObject())
          p.velCurves[i] = piecewiseCurveFromJson(arr.at(i).toObject());
        else
          p.velCurves[i] = PiecewiseCurve{};
      }
    }

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