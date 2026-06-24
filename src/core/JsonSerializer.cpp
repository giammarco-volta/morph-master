//#include "JsonSerializer.h"
//#include "CategoryClassifier.h"
//
//#include <fstream>
//#include <sstream>
//
//namespace
//{
//std::string escapeJson(const std::string& s)
//{
//    std::string out;
//    out.reserve(s.size() + 16);
//
//    for (unsigned char ch : s)
//    {
//        switch (ch)
//        {
//        case '\"': out += "\\\""; break;
//        case '\\': out += "\\\\"; break;
//        case '\b': out += "\\b"; break;
//        case '\f': out += "\\f"; break;
//        case '\n': out += "\\n"; break;
//        case '\r': out += "\\r"; break;
//        case '\t': out += "\\t"; break;
//        default:
//            if (ch < 0x20)
//            {
//                static const char* hex = "0123456789ABCDEF";
//                out += "\\u00";
//                out += hex[(ch >> 4) & 0x0F];
//                out += hex[ch & 0x0F];
//            }
//            else
//            {
//                out += static_cast<char>(ch);
//            }
//            break;
//        }
//    }
//
//    return out;
//}
//
//void indent(std::ostringstream& oss, int level)
//{
//    for (int i = 0; i < level; ++i)
//        oss << "  ";
//}
//
//void writeString(std::ostringstream& oss, const std::string& s)
//{
//    oss << "\"" << escapeJson(s) << "\"";
//}
//
//void writeProgram(std::ostringstream& oss, const ProgramEntry& p, int level)
//{
//    indent(oss, level); oss << "{\n";
//
//    indent(oss, level + 1); oss << "\"category\": ";
//    writeString(oss, toString(p.category)); oss << ",\n";
//
//    indent(oss, level + 1); oss << "\"bankName\": ";
//    writeString(oss, p.bankName); oss << ",\n";
//
//    indent(oss, level + 1); oss << "\"name\": ";
//    writeString(oss, p.name); oss << ",\n";
//
//    indent(oss, level + 1); oss << "\"msb\": " << p.msb << ",\n";
//    indent(oss, level + 1); oss << "\"lsb\": " << p.lsb << ",\n";
//    indent(oss, level + 1); oss << "\"program\": " << p.program << "\n";
//
//    indent(oss, level); oss << "}";
//}
//
//void writeInstrument(std::ostringstream& oss, const InstrumentDefinition& def, int level)
//{
//    indent(oss, level); oss << "{\n";
//
//    indent(oss, level + 1); oss << "\"deviceName\": ";
//    writeString(oss, def.deviceName); oss << ",\n";
//
//    indent(oss, level + 1); oss << "\"programs\": [\n";
//
//    for (size_t i = 0; i < def.programs.size(); ++i)
//    {
//        writeProgram(oss, def.programs[i], level + 2);
//        if (i + 1 < def.programs.size()) oss << ",";
//        oss << "\n";
//    }
//
//    indent(oss, level + 1); oss << "]\n";
//
//    indent(oss, level); oss << "}";
//}
//
//bool writeFile(const std::string& path, const std::string& content)
//{
//    std::ofstream out(path, std::ios::binary);
//    if (!out) return false;
//
//    out.write(content.data(), static_cast<std::streamsize>(content.size()));
//    return static_cast<bool>(out);
//}
//}
//
//namespace JsonSerializer
//{
//bool saveToFile(const InstrumentDefinition& def, const std::string& filePath)
//{
//    return writeFile(filePath, toJson(def));
//}
//
//bool saveAllToFile(const std::vector<InstrumentDefinition>& defs, const std::string& filePath)
//{
//    return writeFile(filePath, toJson(defs));
//}
//
//std::string toJson(const InstrumentDefinition& def)
//{
//    std::ostringstream oss;
//    writeInstrument(oss, def, 0);
//    oss << "\n";
//    return oss.str();
//}
//
//std::string toJson(const std::vector<InstrumentDefinition>& defs)
//{
//    std::ostringstream oss;
//    oss << "{\n";
//    indent(oss, 1); oss << "\"instruments\": [\n";
//
//    for (size_t i = 0; i < defs.size(); ++i)
//    {
//        writeInstrument(oss, defs[i], 2);
//        if (i + 1 < defs.size()) oss << ",";
//        oss << "\n";
//    }
//
//    indent(oss, 1); oss << "]\n";
//    oss << "}\n";
//
//    return oss.str();
//}
//}

#include "JsonSerializer.h"
#include "CategoryClassifier.h"

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
    obj["category"] = QString::fromStdString(category_name[static_cast<uint8_t>(p.category)]);
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