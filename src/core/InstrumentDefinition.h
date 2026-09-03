#pragma once

#include <string>
#include <vector>

struct ProgramEntry
{
  std::string bankName;
  std::string name;
  int msb = -1;
  int lsb = -1;
  int program = -1;
};

struct InstrumentDefinition
{
  std::string deviceName;
  std::vector<ProgramEntry> programs;
};

enum class PatchPolicy { Manual, Instrument };