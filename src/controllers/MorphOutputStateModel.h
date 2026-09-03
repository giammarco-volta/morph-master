#pragma once

#include <QAbstractListModel>
#include <QVariantList>

#include <array>

#include "../core/MorphOutputId.h"
#include "../core/Presets.h"

class MorphOutputStateModel : public QAbstractListModel
{
  Q_OBJECT

public:
  enum Role
  {
    MorphOutputIdRole = Qt::UserRole + 1,
    NameRole,
    SpecificNameRole,
    GainRole,
    AssignedTrackIndexesRole,
    AssignedTrackCountRole,
    TrackMaskRole,
    MutedRole,
    SoloRole
  };
  Q_ENUM(Role)

  explicit MorphOutputStateModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setAssignments(const std::array<TrackPresetData, 16>& tracks);
  void setSpecificNames(const std::array<MorphOutputPresetData, 8>& outputs);
  void setMuteSoloMasks(uint8_t muteMask, uint8_t soloMask);

public slots:
  void setGain(int morphOutputIndex, double gain);
  void setGains(const QVariantList& gains);
  void resetGains();

private:
  struct OutputState
  {
    QString specificName;
    double gain = 0.0;
    QVariantList assignedTrackIndexes;
    int trackMask = 0;
    bool muted = false;
    bool solo = false;
  };

  static QString outputName(MorphOutputId id);

  std::array<OutputState, static_cast<int>(MorphOutputId::None)> states_{};
};
