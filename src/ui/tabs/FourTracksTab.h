#pragma once

#include <QWidget>

#include <array>
#include <cstdint>
#include <vector>

#include "../../core/ExpressionCurveId.h"
#include "../../core/InstrumentDefinition.h"
#include "../../core/TrackGroupId.h"

class QGridLayout;
class TrackStripWidget;
class MainWindow;
struct TrackPresetData;

enum class GroupPlacement : uint8_t;

class FourTracksTab : public QWidget
{
  Q_OBJECT

public:
  explicit FourTracksTab(MainWindow* parent = nullptr, uint8_t trackOffset = 0);

  uint8_t trackOffset() const { return trackOffset_; }
  uint8_t trackIndex(uint8_t localIndex) const { return uint8_t(trackOffset_ + localIndex); }

  TrackGroupId getGroupIdByAbsTrack(uint8_t abstrk) const;
  TrackGroupId getGroupIdByRelTrack(uint8_t loctrk) const;

  void removeTrackFromGroup(uint8_t abstrk, TrackGroupId groupId);
  void assignTrackToGroup(uint8_t abstrk, TrackGroupId groupId);

  // Restituisce le tracce attive contenute dal tab, nell'ordine visivo.
  std::vector<uint8_t> trackIndices() const;

  ExpressionCurveId keyExprCurveId(uint8_t abstrk) const;
  ExpressionCurveId velExprCurveId(uint8_t abstrk) const;
  int8_t timbre1Value(uint8_t abstrk) const;
  int8_t timbre2Value(uint8_t abstrk) const;

  void resetTrackMidiWidgets(uint8_t abstrk);
  void setTrackPresetData(const TrackPresetData& preset, uint8_t abstrk);
  void getTrackPresetData(TrackPresetData& preset, uint8_t abstrk) const;

  void setPatchPolicy(PatchPolicy policy);
  void setInstrumentDefinition(const InstrumentDefinition* def);

  TrackStripWidget* trackWidget(uint8_t abstrk) const;

public slots:
  void onInstrumentProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program); // from TrackStripWidget
  void onManualProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program);     // from TrackStripWidget

signals:
  void instrumentProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program); // to MainWindow
  void manualProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program);     // to MainWindow

private:
  static constexpr int kTrackCount = 4;

  void createTrackWidgets(MainWindow* mainWindow);

private:
  uint8_t trackOffset_ = 0;
  std::array<TrackStripWidget*, kTrackCount> trackWidgets_{};
  QGridLayout* grid_ = nullptr;

  PatchPolicy patchPolicy_ = PatchPolicy::Manual;
  const InstrumentDefinition* instrumentDefinition_ = nullptr;
};
