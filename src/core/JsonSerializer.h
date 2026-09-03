#pragma once

#include "InstrumentDefinition.h"

#include <QString>
#include <string>
#include <vector>

namespace JsonSerializer
{
  bool saveToFile(const InstrumentDefinition& def, const QString& filePath);
  bool saveAllToFile(const std::vector<InstrumentDefinition>& defs, const QString& filePath);

  std::string toJson(const InstrumentDefinition& def);
  std::string toJson(const std::vector<InstrumentDefinition>& defs);
}