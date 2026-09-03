#include "JsonSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace
{
  QJsonObject toJsonObject(const ProgramEntry& p)
  {
    QJsonObject obj;
    obj["bankName"] = QString::fromStdString(p.bankName);
    obj["name"] = QString::fromStdString(p.name);
    obj["msb"] = p.msb;
    obj["lsb"] = p.lsb;
    obj["program"] = p.program;
    return obj;
  }

  QJsonObject toJsonObject(const InstrumentDefinition& def)
  {
    QJsonObject obj;
    obj["deviceName"] = QString::fromStdString(def.deviceName);

    QJsonArray programsArray;
    for (const auto& p : def.programs)
      programsArray.append(toJsonObject(p));

    obj["programs"] = programsArray;
    return obj;
  }

  QByteArray toJsonBytes(const InstrumentDefinition& def)
  {
    QJsonDocument doc(toJsonObject(def));
    return doc.toJson(QJsonDocument::Indented);
  }

  QByteArray toJsonBytes(const std::vector<InstrumentDefinition>& defs)
  {
    QJsonArray instrumentsArray;
    for (const auto& def : defs)
      instrumentsArray.append(toJsonObject(def));

    QJsonObject root;
    root["instruments"] = instrumentsArray;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented);
  }
}

namespace JsonSerializer
{
  bool saveToFile(const InstrumentDefinition& def, const QString& filePath)
  {
    QFile out(filePath);
    if (!out.open(QIODevice::WriteOnly))
      return false;

    const QByteArray json = toJsonBytes(def);
    const qint64 written = out.write(json);
    out.close();

    return written == json.size();
  }

  bool saveAllToFile(const std::vector<InstrumentDefinition>& defs, const QString& filePath)
  {
    QFile out(filePath);
    if (!out.open(QIODevice::WriteOnly))
      return false;

    const QByteArray json = toJsonBytes(defs);
    const qint64 written = out.write(json);
    out.close();

    return written == json.size();
  }

  std::string toJson(const InstrumentDefinition& def)
  {
    const QByteArray json = toJsonBytes(def);
    return std::string(json.constData(), static_cast<size_t>(json.size()));
  }

  std::string toJson(const std::vector<InstrumentDefinition>& defs)
  {
    const QByteArray json = toJsonBytes(defs);
    return std::string(json.constData(), static_cast<size_t>(json.size()));
  }
}