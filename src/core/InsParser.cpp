#include "InsParser.h"
#include "CategoryClassifier.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <charconv>

#include <QFile>
#include <QString>
#include <QStringConverter>

namespace
{
  QString decodeInsText(const QByteArray& data)
  {
    // 1) BOM UTF-8
    if (data.startsWith("\xEF\xBB\xBF"))
      return QString::fromUtf8(data.constData() + 3, data.size() - 3);

    // 2) UTF-8 puro
    {
      QStringDecoder dec(QStringDecoder::Utf8);
      const QString s = dec.decode(data);
      if (!dec.hasError())
        return s;
    }

    // 3) Shift-JIS / CP932: necessario per parecchi .ins giapponesi
    {
      QStringDecoder dec("Shift-JIS");
      const QString s = dec.decode(data);
      if (!dec.hasError())
        return s;
    }

    // 4) fallback Windows-1252 / Latin1-ish
    {
      QStringDecoder dec(QStringDecoder::Latin1);
      const QString s = dec.decode(data);
      if (!dec.hasError())
        return s;
    }

    // 5) ultimissimo fallback
    return QString::fromLatin1(data);
  }
}

namespace
{
  enum class MajorSection
  {
    None,
    PatchNames,
    InstrumentDefinitions
  };

  std::string trim(const std::string& s)
  {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
      ++b;

    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
      --e;

    return s.substr(b, e - b);
  }

  std::string toLowerAscii(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
  }

  bool startsWith(const std::string& s, const std::string& prefix)
  {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
  }

  bool isCommentOrEmpty(const std::string& s)
  {
    return s.empty() || s[0] == ';' || s[0] == '#';
  }

  std::pair<std::string, std::string> splitKeyValue(const std::string& line)
  {
    const auto pos = line.find('=');
    if (pos == std::string::npos)
      return { trim(line), "" };

    return { trim(line.substr(0, pos)), trim(line.substr(pos + 1)) };
  }

  bool tryParseInt(const std::string& s, int& value)
  {
    if (s.empty())
      return false;

    const char* begin = s.data();
    const char* end = s.data() + s.size();

    int v = 0;
    auto result = std::from_chars(begin, end, v, 10);

    if (result.ec != std::errc() || result.ptr != end)
      return false;

    value = v;
    return true;
  }

  bool tryParseStarOrInt(const std::string& s, bool& isStar, int& value)
  {
    if (s == "*")
    {
      isStar = true;
      value = -1;
      return true;
    }

    isStar = false;
    return tryParseInt(s, value);
  }

  bool parseBracketContent(const std::string& key, const std::string& prefix, std::string& inside)
  {
    if (!startsWith(key, prefix))
      return false;

    const auto lb = key.find('[', prefix.size());
    const auto rb = key.find(']', lb == std::string::npos ? 0 : lb + 1);
    if (lb == std::string::npos || rb == std::string::npos || rb <= lb + 1)
      return false;

    inside = trim(key.substr(lb + 1, rb - lb - 1));
    return true;
  }

  bool tryParsePatchIndex(const std::string& key, bool& isWildcard, int& bankIndex)
  {
    std::string inside;
    if (!parseBracketContent(key, "Patch", inside))
      return false;

    return tryParseStarOrInt(inside, isWildcard, bankIndex);
  }

  struct KeySelector
  {
    bool bankWildcard = false;
    int bankIndex = -1;
    bool programWildcard = false;
    int program = -1;
  };

  bool tryParseKeySelector(const std::string& key, KeySelector& selector)
  {
    std::string inside;
    if (!parseBracketContent(key, "Key", inside))
      return false;

    const auto comma = inside.find(',');
    if (comma == std::string::npos)
      return false;

    const std::string a = trim(inside.substr(0, comma));
    const std::string b = trim(inside.substr(comma + 1));

    return tryParseStarOrInt(a, selector.bankWildcard, selector.bankIndex) &&
      tryParseStarOrInt(b, selector.programWildcard, selector.program);
  }

  bool tryParseDrumSelector(const std::string& key, KeySelector& selector)
  {
    std::string inside;
    if (!parseBracketContent(key, "Drum", inside))
      return false;

    const auto comma = inside.find(',');
    if (comma == std::string::npos)
      return false;

    const std::string a = trim(inside.substr(0, comma));
    const std::string b = trim(inside.substr(comma + 1));

    return tryParseStarOrInt(a, selector.bankWildcard, selector.bankIndex) &&
      tryParseStarOrInt(b, selector.programWildcard, selector.program);
  }

  struct PatchListSection
  {
    std::string name;
    std::optional<std::string> basedOn;
    std::map<int, std::string> programs;
  };

  struct InstrumentSection
  {
    std::string name;
    std::optional<std::string> defaultPatchList;
    std::map<int, std::string> patchListByBank;
  };

  bool looksLikePatchProgramKey(const std::string& key)
  {
    int v = -1;
    return tryParseInt(key, v);
  }

  bool deduceSectionType(const std::string& key, MajorSection major, bool currentlyUnknown, bool& isPatchList, bool& isInstrument)
  {
    isPatchList = false;
    isInstrument = false;

    if (major == MajorSection::PatchNames)
    {
      isPatchList = true;
      return true;
    }

    if (major == MajorSection::InstrumentDefinitions)
    {
      isInstrument = true;
      return true;
    }

    if (looksLikePatchProgramKey(key) || key == "BasedOn")
    {
      isPatchList = true;
      return true;
    }

    if (startsWith(key, "Patch[") || startsWith(key, "Key[") || startsWith(key, "Drum[") || key == "BankSelMethod")
    {
      isInstrument = true;
      return true;
    }

    if (currentlyUnknown)
      return false;

    return true;
  }

  bool resolvePatchListRecursive(
    const std::string& name,
    const std::unordered_map<std::string, PatchListSection>& allPatchLists,
    std::map<int, std::string>& outPrograms,
    std::set<std::string>& recursionStack,
    std::vector<InsParser::ParseError>& warnings)
  {
    const auto it = allPatchLists.find(name);
    if (it == allPatchLists.end())
    {
      warnings.push_back({ 0, "Missing patch list referenced by BasedOn: [" + name + "]" });
      return false;
    }

    if (recursionStack.find(name) != recursionStack.end())
    {
      warnings.push_back({ 0, "Circular BasedOn chain detected at patch list: [" + name + "]" });
      return false;
    }

    recursionStack.insert(name);

    if (it->second.basedOn)
    {
      std::map<int, std::string> basePrograms;
      if (resolvePatchListRecursive(*it->second.basedOn, allPatchLists, basePrograms, recursionStack, warnings))
      {
        outPrograms = std::move(basePrograms);
      }
    }

    for (const auto& kv : it->second.programs)
      outPrograms[kv.first] = kv.second;

    recursionStack.erase(name);
    return true;
  }

  std::map<int, std::string> resolvePatchList(
    const std::string& name,
    const std::unordered_map<std::string, PatchListSection>& allPatchLists,
    std::vector<InsParser::ParseError>& warnings)
  {
    std::map<int, std::string> outPrograms;
    std::set<std::string> recursionStack;
    resolvePatchListRecursive(name, allPatchLists, outPrograms, recursionStack, warnings);
    return outPrograms;
  }

  void sortPrograms(InstrumentDefinition& def)
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
}

std::optional<InsParser::ParseResult> InsParser::parseText(const std::string& text, ParseError& fatalError) const
{
  ParseResult result;

  std::unordered_map<std::string, PatchListSection> patchLists;
  std::vector<InstrumentSection> instruments;

  MajorSection majorSection = MajorSection::None;
  std::string currentSectionName;

  enum class ConcreteSection
  {
    None,
    Unknown,
    PatchList,
    Instrument
  };

  ConcreteSection concreteSection = ConcreteSection::None;

  std::istringstream iss(text);
  std::string rawLine;
  int lineNo = 0;

  while (std::getline(iss, rawLine))
  {
    ++lineNo;

    if (!rawLine.empty() && rawLine.back() == '\r')
      rawLine.pop_back();

    std::string line = trim(rawLine);
    if (isCommentOrEmpty(line))
      continue;

    if (line[0] == '.')
    {
      const std::string low = toLowerAscii(line);
      if (low == ".patch names")
        majorSection = MajorSection::PatchNames;
      else if (low == ".instrument definitions")
        majorSection = MajorSection::InstrumentDefinitions;
      else
        majorSection = MajorSection::None;

      currentSectionName.clear();
      concreteSection = ConcreteSection::None;
      continue;
    }

    if (line.front() == '[' && line.back() == ']')
    {
      currentSectionName = trim(line.substr(1, line.size() - 2));
      concreteSection = ConcreteSection::Unknown;

      if (majorSection == MajorSection::PatchNames)
      {
        patchLists[currentSectionName] = PatchListSection{ currentSectionName, std::nullopt, {} };
        concreteSection = ConcreteSection::PatchList;
      }
      else if (majorSection == MajorSection::InstrumentDefinitions)
      {
        instruments.push_back(InstrumentSection{ currentSectionName, std::nullopt, {} });
        concreteSection = ConcreteSection::Instrument;
      }

      continue;
    }

    if (currentSectionName.empty())
      continue;

    const auto [key, value] = splitKeyValue(line);
    if (key.empty())
      continue;

    bool isPatchList = false;
    bool isInstrument = false;
    if (!deduceSectionType(key, majorSection, concreteSection == ConcreteSection::Unknown, isPatchList, isInstrument))
    {
      result.warnings.push_back({ lineNo, "Unable to deduce section type for line: " + line });
      continue;
    }

    if (concreteSection == ConcreteSection::Unknown)
    {
      if (isPatchList)
      {
        patchLists[currentSectionName] = PatchListSection{ currentSectionName, std::nullopt, {} };
        concreteSection = ConcreteSection::PatchList;
      }
      else if (isInstrument)
      {
        instruments.push_back(InstrumentSection{ currentSectionName, std::nullopt, {} });
        concreteSection = ConcreteSection::Instrument;
      }
    }

    if (concreteSection == ConcreteSection::PatchList)
    {
      auto it = patchLists.find(currentSectionName);
      if (it == patchLists.end())
      {
        patchLists[currentSectionName] = PatchListSection{ currentSectionName, std::nullopt, {} };
        it = patchLists.find(currentSectionName);
      }

      if (key == "BasedOn")
      {
        it->second.basedOn = value;
        continue;
      }

      int program = -1;
      if (tryParseInt(key, program))
      {
        it->second.programs[program] = value;
        continue;
      }

      result.warnings.push_back({ lineNo, "Unrecognized patch-list line ignored: " + line });
      continue;
    }

    if (concreteSection == ConcreteSection::Instrument)
    {
      if (instruments.empty() || instruments.back().name != currentSectionName)
      {
        instruments.push_back(InstrumentSection{ currentSectionName, std::nullopt, {} });
      }

      auto& inst = instruments.back();

      if (key == "BankSelMethod")
      {
        continue;
      }

      bool wildcard = false;
      int bankIndex = -1;
      if (tryParsePatchIndex(key, wildcard, bankIndex))
      {
        if (wildcard)
          inst.defaultPatchList = value;
        else
          inst.patchListByBank[bankIndex] = value;

        continue;
      }

      KeySelector ks;
      if (tryParseKeySelector(key, ks))
      {
        continue;
      }

      if (tryParseDrumSelector(key, ks))
      {
        continue;
      }

      result.warnings.push_back({ lineNo, "Unrecognized instrument-definition line ignored: " + line });
      continue;
    }
  }

  if (patchLists.empty() && instruments.empty())
  {
    fatalError = { 0, "No usable sections found in .ins text." };
    return std::nullopt;
  }

  std::unordered_map<std::string, std::map<int, std::string>> resolvedPatchLists;
  for (const auto& kv : patchLists)
  {
    resolvedPatchLists[kv.first] = resolvePatchList(kv.first, patchLists, result.warnings);
  }

  for (const auto& inst : instruments)
  {
    InstrumentDefinition def;
    def.deviceName = inst.name;

    std::set<int> bankIndices;
    for (const auto& kv : inst.patchListByBank)
      bankIndices.insert(kv.first);

    if (bankIndices.empty() && inst.defaultPatchList)
      bankIndices.insert(0);

    for (int bankIndex : bankIndices)
    {
      std::string patchListName;

      const auto itBank = inst.patchListByBank.find(bankIndex);
      if (itBank != inst.patchListByBank.end())
      {
        patchListName = itBank->second;
      }
      else if (inst.defaultPatchList)
      {
        patchListName = *inst.defaultPatchList;
      }
      else
      {
        result.warnings.push_back({ 0, "Instrument [" + inst.name + "] has bank without patch list." });
        continue;
      }

      const auto itResolved = resolvedPatchLists.find(patchListName);
      if (itResolved == resolvedPatchLists.end())
      {
        result.warnings.push_back({ 0, "Instrument [" + inst.name + "] references unknown patch list [" + patchListName + "]" });
        continue;
      }

      const int msb = bankIndex / 128;
      const int lsb = bankIndex % 128;

      for (const auto& prog : itResolved->second)
      {
        ProgramEntry entry;
        entry.bankName = patchListName;
        entry.name = prog.second;
        entry.program = prog.first;
        entry.msb = msb;
        entry.lsb = lsb;
        entry.category = classifyProgramCategory(entry.name, entry.msb, entry.lsb, entry.program);
        def.programs.push_back(std::move(entry));
      }
    }

    sortPrograms(def);
    result.instruments.push_back(std::move(def));
  }

  return result;
}

std::optional<InsParser::ParseResult> InsParser::parseFile(const QString& filePath, ParseError& fatalError) const
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly))
  {
    fatalError = { 0, "Cannot open file: " + filePath.toStdString() };
    return std::nullopt;
  }

  const QByteArray rawData = file.readAll();
  file.close();

  const QString decoded = decodeInsText(rawData);
  const QByteArray utf8 = decoded.toUtf8();

  return parseText(std::string(utf8.constData(), static_cast<size_t>(utf8.size())), fatalError);
}