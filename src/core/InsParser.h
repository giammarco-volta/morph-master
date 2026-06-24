#pragma once

#include "InstrumentDefinition.h"

#include <optional>
#include <string>
#include <vector>

class QString;

class InsParser
{
public:
  struct ParseError
  {
    int line = 0;
    std::string message;
  };

  struct ParseResult
  {
    std::vector<InstrumentDefinition> instruments;
    std::vector<ParseError> warnings;
  };

  std::optional<ParseResult> parseFile(const QString& filePath, ParseError& fatalError) const;
  std::optional<ParseResult> parseText(const std::string& text, ParseError& fatalError) const;
};