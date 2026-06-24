#pragma once

#include "InstrumentDefinition.h"
#include <string>

ProgramCategory classifyProgramCategory(const std::string& patchName, uint8_t msb, uint8_t lsb, uint8_t program);
