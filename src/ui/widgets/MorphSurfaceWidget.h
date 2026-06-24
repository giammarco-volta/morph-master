#pragma once

#include <QWidget>
#include <QImage>
#include <array>
#include <list>
#include <QTimer>
#include <QElapsedTimer>
#include <QIcon>
#include "../../core/TrackGroupId.h"


enum class BadgeSymbol
{
  Treble,
  Bass,
  Piano,
  Forte
};

using BadgeSymbols = QVector<BadgeSymbol>;


//---------------------------------------
class MorphSurfaceWidget : public QWidget
//---------------------------------------
{
  Q_OBJECT

public:
  explicit MorphSurfaceWidget(QWidget* parent = nullptr);

  void setActiveGroups(const std::array<std::list<uint8_t>, (uint8_t)TrackGroupId::Count>& activeGroups);

  void onNoteOn(uint8_t note, uint8_t velocity);
  void onNoteOff(uint8_t note);

  void setXRange(uint8_t minNote, uint8_t maxNote)
  {
    minX_ = minNote;
    maxX_ = maxNote;
  }

signals:
  void groupBadgeTouched(TrackGroupId groupIndex);
  void groupBadgeTrackMaskEdited(TrackGroupId groupIndex, uint16_t trackMask);

  // Emitted only by the interactive surface test feature.
  // MainWindow is responsible for translating these into real MIDI messages.
  void surfaceTestNoteOn(uint8_t note, uint8_t velocity);
  void surfaceTestNoteOff(uint8_t note);

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;//Windows
  void mouseMoveEvent(QMouseEvent* event) override;//Windows
  void mouseReleaseEvent(QMouseEvent* event) override;//Windows
  bool event(QEvent* event) override;//Android and iOS

private:
  void rebuildImage();

  BadgeSymbols badgeSymbols(uint8_t groupIndex) const;
  qreal badgeSymbolWidth(const QVector<BadgeSymbol>& parts, const QFontMetricsF& textFm) const;

  void drawSymbols(QPainter& p,
    const QVector<BadgeSymbol>& parts,
    QPointF pos,
    qreal baseline,
    const QColor& color,
    const QFont& textFont) const;

  QPoint badgeCenter(uint8_t groupIndex) const;
  QColor sampleSurfaceColor(const QPoint& pt) const;
  QString badgeIconLabel(uint8_t groupIndex) const;
  QString badgeTrackList(uint8_t groupIndex) const;
  void drawBadge(QPainter& p, uint8_t groupIndex);
  void drawExpandedBadgePanel(QPainter& p);
  uint16_t trackMaskForGroup(uint8_t groupIndex) const;
  void openExpandedBadge(uint8_t groupIndex);
  void closeExpandedBadgeAndNotify();
  void clearExpandedBadgeGeometry();
  QVector<int> visibleXMarks() const;

  void startAnimationIfNeeded();
  void stopAnimationIfPossible();
  void updateFadeStates();
  bool hasActiveFades() const;

  QPointF dotPositionFor(int midiNote, int velocity) const;

  bool handlePointerPress(const QPointF& pos);

  uint8_t noteFromSurfacePoint(const QPointF& pos) const;
  uint8_t velocityFromSurfacePoint(const QPointF& pos) const;
  void startSurfaceTestNote(const QPointF& pos);
  void updateSurfaceTestNote(const QPointF& pos);
  void stopSurfaceTestNote();

private slots:
  void onAnimationTick();

private:
  static constexpr int kNumNotes = 128;
  static constexpr int kFadeDurationMs = 300;   // prova 500 o 1000
  static constexpr int kTimerIntervalMs = 20;   // ~60 fps; puoi anche usare 20 o 30

  struct NoteDotState
  {
    bool isOn = false;          // nota attualmente premuta
    bool fading = false;        // sta svanendo dopo il note off
    float alpha = 0.0f;         // 0.0 .. 1.0
    qint64 fadeStartMs = 0;     // ms dall'orologio monotono
    uint8_t key = 0;
    uint8_t velocity = 0;
  };

  std::array<QRectF, 8> badgeRects_;
  QRectF expandedPanelRect_;
  QRectF expandedHeaderRect_;
  std::array<QRectF, 16> expandedTrackRects_;
  int expandedGroupIndex_ = -1;
  uint16_t expandedTrackMask_ = 0;

  QTimer animationTimer_;
  QElapsedTimer clock_;

  QImage image_;
  std::array<std::list<uint8_t>, (uint8_t)TrackGroupId::Count> activeGroups_;

  std::list<NoteDotState> currentNotes_;

  bool surfaceTestNoteActive_ = false;
  uint8_t surfaceTestNote_ = 0;
  uint8_t surfaceTestVelocity_ = 0;

  uint8_t minX_ = 0;
  uint8_t maxX_ = 127;

  QIcon trebleClefIcon_;
  QIcon bassClefIcon_;
};