#pragma once

#include "InstrumentDefinition.h"

#include <QStringList>
#include <vector>
#include <string>

class InstrumentDatabase
{
public:
  bool loadFromJsonFile(const QString& filePath, QString* errorMessage = nullptr);

  const std::vector<InstrumentDefinition>& instruments() const { return instruments_; }

  QStringList instrumentNames() const;

  const InstrumentDefinition* findByName(const QString& name) const;

private:
  std::vector<InstrumentDefinition> instruments_;
};