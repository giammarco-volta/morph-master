#include "InstrumentDatabase.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace
{

QString normalizedInstrumentName(const std::string& name)
{
  return QString::fromStdString(name).trimmed().toCaseFolded();
}

using ProgramKey = std::tuple<int, int, int>; // msb, lsb, program

ProgramKey programKey(const ProgramEntry& p)
{
  return ProgramKey{ p.msb, p.lsb, p.program };
}

void sortProgramsByMidiAddress(InstrumentDefinition& def)
{
  std::sort(def.programs.begin(), def.programs.end(),
    [](const ProgramEntry& a, const ProgramEntry& b)
    {
      if (a.msb != b.msb) return a.msb < b.msb;
      if (a.lsb != b.lsb) return a.lsb < b.lsb;
      if (a.program != b.program) return a.program < b.program;
      if (a.bankName != b.bankName) return a.bankName < b.bankName;
      return a.name < b.name;
    });
}

bool hasProgramWithSameMidiAddress(const InstrumentDefinition& def, const ProgramEntry& candidate)
{
  const ProgramKey key = programKey(candidate);

  return std::any_of(def.programs.begin(), def.programs.end(),
    [&](const ProgramEntry& existing)
    {
      return programKey(existing) == key;
    });
}

void mergeInstrument(std::vector<InstrumentDefinition>& instruments,
                     std::map<QString, size_t>& indexByNormalizedName,
                     InstrumentDefinition&& source)
{
  const QString normalizedName = normalizedInstrumentName(source.deviceName);
  if (normalizedName.isEmpty())
    return;

  const auto it = indexByNormalizedName.find(normalizedName);
  if (it == indexByNormalizedName.end())
  {
    sortProgramsByMidiAddress(source);
    indexByNormalizedName[normalizedName] = instruments.size();
    instruments.push_back(std::move(source));
    return;
  }

  InstrumentDefinition& target = instruments[it->second];
  for (auto& program : source.programs)
  {
    if (!hasProgramWithSameMidiAddress(target, program))
      target.programs.push_back(std::move(program));
  }

  sortProgramsByMidiAddress(target);
}
}

bool InstrumentDatabase::loadFromJsonFile(const QString& filePath, QString* errorMessage)
{
  instruments_.clear();

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly))
  {
    if (errorMessage)
      *errorMessage = QString("Cannot open file: %1").arg(filePath);
    return false;
  }

  const QByteArray data = file.readAll();
  file.close();

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError)
  {
    qDebug() << "JSON parse error:" << parseError.errorString();
    qDebug() << "Offset:" << parseError.offset;

    const int start = std::max(0, int(parseError.offset) - 80);
    const int len = std::min(160, int(data.size()) - start);
    qDebug() << "Context:" << data.mid(start, len);

    return false;
  }

  if (!doc.isObject())
  {
    if (errorMessage)
      *errorMessage = "Root JSON is not an object.";
    return false;
  }

  const QJsonObject root = doc.object();
  const QJsonValue instrumentsVal = root.value("instruments");
  if (!instrumentsVal.isArray())
  {
    if (errorMessage)
      *errorMessage = "Missing or invalid 'instruments' array.";
    return false;
  }

  const QJsonArray instrumentsArray = instrumentsVal.toArray();

  std::map<QString, size_t> indexByNormalizedName;

  for (const QJsonValue& instrVal : instrumentsArray)
  {
    if (!instrVal.isObject())
      continue;

    const QJsonObject instrObj = instrVal.toObject();

    InstrumentDefinition def;
    def.deviceName = instrObj.value("deviceName").toString().trimmed().toStdString();

    const QJsonValue programsVal = instrObj.value("programs");
    if (programsVal.isArray())
    {
      const QJsonArray programsArray = programsVal.toArray();
      def.programs.reserve(programsArray.size());

      for (const QJsonValue& progVal : programsArray)
      {
        if (!progVal.isObject())
          continue;

        const QJsonObject progObj = progVal.toObject();

        ProgramEntry p;
        p.bankName = progObj.value("bankName").toString().toStdString();
        p.name = progObj.value("name").toString().toStdString();
        p.msb = progObj.value("msb").toInt(-1);
        p.lsb = progObj.value("lsb").toInt(-1);
        p.program = progObj.value("program").toInt(-1);

        def.programs.push_back(std::move(p));
      }
    }

    mergeInstrument(instruments_, indexByNormalizedName, std::move(def));
  }

  std::sort(instruments_.begin(), instruments_.end(),
    [](const InstrumentDefinition& a, const InstrumentDefinition& b)
    {
      return QString::fromStdString(a.deviceName).localeAwareCompare(QString::fromStdString(b.deviceName)) < 0;
    });

  return true;
}

QStringList InstrumentDatabase::instrumentNames() const
{
  QStringList names;
  std::set<QString> alreadyAdded;

  for (const auto& i : instruments_)
  {
    const QString visibleName = QString::fromStdString(i.deviceName).trimmed();
    const QString key = visibleName.toCaseFolded();

    if (visibleName.isEmpty() || alreadyAdded.find(key) != alreadyAdded.end())
      continue;

    alreadyAdded.insert(key);
    names << visibleName;
  }

  names.sort(Qt::CaseInsensitive);
  return names;
}

const InstrumentDefinition* InstrumentDatabase::findByName(const QString& name) const
{
  const QString target = name.trimmed().toCaseFolded();

  for (const auto& i : instruments_)
    if (normalizedInstrumentName(i.deviceName) == target)
      return &i;

  return nullptr;
}
