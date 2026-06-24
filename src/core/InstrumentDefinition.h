#pragma once

#include <string>
#include <vector>

enum class ProgramCategory : uint8_t
{
  Piano,                  
  ElectricPiano,
  ChromaticPercussion,
  Accordion,
  Organ,
  Guitar,
  Bass,
  Strings,
  Ensemble,
  Brass,
  Reed,
  Pipe,
  SynthLead,
  SynthPad,
  SynthEffects,
  Ethnic,
  Percussive,
  SoundEffects,
  Other,
  numofCategories
};

const std::string category_name[static_cast<size_t>(ProgramCategory::numofCategories)] =
{
  "Piano",
  "ElectricPiano",
  "ChromaticPercussion",
  "Accordion",
  "Organ",
  "Guitar",
  "Bass",
  "Strings",
  "Ensemble",
  "Brass",
  "Reed",
  "Pipe",
  "SynthLead",
  "SynthPad",
  "SynthEffects",
  "Ethnic",
  "Percussive",
  "SoundEffects",
  "Other"
};

struct ProgramEntry
{
  ProgramCategory category = ProgramCategory::Other;
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