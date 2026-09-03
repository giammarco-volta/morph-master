#include "MorphOutputStateModel.h"

#include <algorithm>
#include <cmath>

MorphOutputStateModel::MorphOutputStateModel(QObject* parent)
  : QAbstractListModel(parent)
{
}

int MorphOutputStateModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(states_.size());
}

QVariant MorphOutputStateModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
    return {};

  const int row = index.row();
  const auto id = static_cast<MorphOutputId>(row);
  const auto& state = states_[row];

  switch (role)
  {
    case Qt::DisplayRole:
    case NameRole:
      return outputName(id);

    case SpecificNameRole:
      return state.specificName;

    case MorphOutputIdRole:
      return row;

    case GainRole:
      return state.gain;

    case AssignedTrackIndexesRole:
      return state.assignedTrackIndexes;

    case AssignedTrackCountRole:
      return state.assignedTrackIndexes.size();

    case TrackMaskRole:
      return state.trackMask;

    case MutedRole:
      return state.muted;

    case SoloRole:
      return state.solo;

    default:
      return {};
  }
}

QHash<int, QByteArray> MorphOutputStateModel::roleNames() const
{
  return {
    { MorphOutputIdRole, "morphOutputId" },
    { NameRole, "name" },
    { SpecificNameRole, "specificName" },
    { GainRole, "gain" },
    { AssignedTrackIndexesRole, "assignedTrackIndexes" },
    { AssignedTrackCountRole, "assignedTrackCount" },
    { TrackMaskRole, "trackMask" },
    { MutedRole, "muted" },
    { SoloRole, "solo" }
  };
}

void MorphOutputStateModel::setSpecificNames(const std::array<MorphOutputPresetData, 8>& outputs)
{
  for (int outputIndex = 0; outputIndex < rowCount(); ++outputIndex)
  {
    const QString newName = outputs[outputIndex].name;
    auto& state = states_[outputIndex];
    if (state.specificName == newName)
      continue;
    state.specificName = newName;
    const QModelIndex changedIndex = index(outputIndex, 0);
    emit dataChanged(changedIndex, changedIndex, { SpecificNameRole });
  }
}

void MorphOutputStateModel::setAssignments(const std::array<TrackPresetData, 16>& tracks)
{
  std::array<QVariantList, static_cast<int>(MorphOutputId::None)> newIndexes;
  std::array<int, static_cast<int>(MorphOutputId::None)> newMasks{};

  for (int trackIndex = 0; trackIndex < static_cast<int>(tracks.size()); ++trackIndex)
  {
    const int outputIndex = static_cast<int>(tracks[trackIndex].morphOutput);

    if (outputIndex < 0 || outputIndex >= static_cast<int>(MorphOutputId::None))
      continue;

    // QML uses human-readable track numbers (1..16).
    newIndexes[outputIndex].append(trackIndex + 1);
    newMasks[outputIndex] |= (1 << trackIndex);
  }

  for (int outputIndex = 0; outputIndex < rowCount(); ++outputIndex)
  {
    auto& state = states_[outputIndex];

    if (state.assignedTrackIndexes == newIndexes[outputIndex]
        && state.trackMask == newMasks[outputIndex])
    {
      continue;
    }

    state.assignedTrackIndexes = newIndexes[outputIndex];
    state.trackMask = newMasks[outputIndex];

    const QModelIndex changedIndex = index(outputIndex, 0);
    emit dataChanged(changedIndex, changedIndex,
                     { AssignedTrackIndexesRole, AssignedTrackCountRole, TrackMaskRole });
  }
}

void MorphOutputStateModel::setMuteSoloMasks(uint8_t muteMask, uint8_t soloMask)
{
  for (int outputIndex = 0; outputIndex < rowCount(); ++outputIndex)
  {
    auto& state = states_[outputIndex];
    const bool muted = (muteMask & (1u << outputIndex)) != 0;
    const bool solo = (soloMask & (1u << outputIndex)) != 0;

    if (state.muted == muted && state.solo == solo)
      continue;

    state.muted = muted;
    state.solo = solo;
    const QModelIndex changedIndex = index(outputIndex, 0);
    emit dataChanged(changedIndex, changedIndex, { MutedRole, SoloRole });
  }
}

void MorphOutputStateModel::setGain(int morphOutputIndex, double gain)
{
  if (morphOutputIndex < 0 || morphOutputIndex >= rowCount())
    return;

  gain = std::clamp(gain, 0.0, 1.0);

  auto& state = states_[morphOutputIndex];

  if (std::abs(state.gain - gain) < 0.0001)
    return;

  state.gain = gain;

  const QModelIndex changedIndex = index(morphOutputIndex, 0);
  emit dataChanged(changedIndex, changedIndex, { GainRole });
}

void MorphOutputStateModel::setGains(const QVariantList& gains)
{
  int firstChanged = -1;
  int lastChanged = -1;

  for (int outputIndex = 0; outputIndex < rowCount(); ++outputIndex)
  {
    const double gain = outputIndex < gains.size()
                      ? std::clamp(gains[outputIndex].toDouble(), 0.0, 1.0)
                      : 0.0;

    auto& state = states_[outputIndex];
    if (std::abs(state.gain - gain) < 0.0001)
      continue;

    state.gain = gain;
    if (firstChanged < 0)
      firstChanged = outputIndex;
    lastChanged = outputIndex;
  }

  if (firstChanged >= 0)
    emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), { GainRole });
}

void MorphOutputStateModel::resetGains()
{
  QVariantList gains;
  gains.reserve(rowCount());
  for (int outputIndex = 0; outputIndex < rowCount(); ++outputIndex)
    gains.append(0.0);
  setGains(gains);
}

QString MorphOutputStateModel::outputName(MorphOutputId id)
{
  switch (id)
  {
    case MorphOutputId::Loud:     return QStringLiteral("Loud");
    case MorphOutputId::HighLoud: return QStringLiteral("High and Loud");
    case MorphOutputId::High:     return QStringLiteral("High");
    case MorphOutputId::HighSoft: return QStringLiteral("High and Soft");
    case MorphOutputId::Soft:     return QStringLiteral("Soft");
    case MorphOutputId::LowSoft:  return QStringLiteral("Low and Soft");
    case MorphOutputId::Low:      return QStringLiteral("Low");
    case MorphOutputId::LowLoud:  return QStringLiteral("Low and Loud");
    case MorphOutputId::None:
    case MorphOutputId::Count:
      break;
  }

  return {};
}
