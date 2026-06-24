#include "MorphSurfaceWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QPen>
#include <QThread>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QStringList>
#include <algorithm>
#include <cmath>

namespace
{
  QString musicFontFamily()
  {
    static const QString family = []() -> QString
    {
      // Put Bravura.otf, or another SMuFL font with the same glyph mapping,
      // in the Qt resource system as :/fonts/Bravura.otf.
      const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Bravura.otf"));
      if (id >= 0)
      {
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty())
          return families.front();
      }

      // Useful during development if Bravura is installed on the machine but
      // has not yet been added to the .qrc file.
      if (QFontDatabase::families().contains(QStringLiteral("Bravura")))
        return QStringLiteral("Bravura");

      return QString();
    }();

    return family;
  }

  QString ucs4(uint codepoint)
  {
    return QString::fromUcs4(&codepoint, 1);
  }
}

//------------------------------------------------------------------------------------------
static QPixmap tintedIconPixmap(const QString& path, const QSize& size, const QColor& color)
//------------------------------------------------------------------------------------------
{
  QPixmap pm = QIcon(path).pixmap(size);
  if (pm.isNull())
    return pm;

  QPixmap tinted(pm.size());
  tinted.fill(Qt::transparent);

  QPainter tp(&tinted);
  tp.drawPixmap(0, 0, pm);
  tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
  tp.fillRect(tinted.rect(), color);
  tp.end();

  return tinted;
}


//-----------------------------------------------------------------------
MorphSurfaceWidget::MorphSurfaceWidget(QWidget* parent) : QWidget(parent)
//-----------------------------------------------------------------------
{
  setAttribute(Qt::WA_AcceptTouchEvents);

  trebleClefIcon_ = QIcon(":/svg/treble-clef.svg");
  bassClefIcon_ = QIcon(":/svg/bass-clef.svg");

  setMinimumSize(300, 300);
  rebuildImage();
  clock_.start();

  animationTimer_.setInterval(kTimerIntervalMs);
  connect(&animationTimer_, &QTimer::timeout, this, &MorphSurfaceWidget::onAnimationTick);
}

//---------------------------------------------------------------------
BadgeSymbols MorphSurfaceWidget::badgeSymbols(uint8_t groupIndex) const
//---------------------------------------------------------------------
{
  switch (groupIndex)
  {
  case 0: return { BadgeSymbol::Forte };
  case 1: return { BadgeSymbol::Treble, BadgeSymbol::Forte };
  case 2: return { BadgeSymbol::Treble };
  case 3: return { BadgeSymbol::Treble, BadgeSymbol::Piano };
  case 4: return { BadgeSymbol::Piano };
  case 5: return { BadgeSymbol::Bass, BadgeSymbol::Piano };
  case 6: return { BadgeSymbol::Bass };
  case 7: return { BadgeSymbol::Bass, BadgeSymbol::Forte };
  default: return {};
  }
}

//--------------------------------------------------------------------------------------------------------------
qreal MorphSurfaceWidget::badgeSymbolWidth(const QVector<BadgeSymbol>& parts, const QFontMetricsF& textFm) const
//--------------------------------------------------------------------------------------------------------------
{
  qreal w = 0.0;

  for (BadgeSymbol part : parts)
  {
    switch (part)
    {
    case BadgeSymbol::Treble: w += 28.0; break;
    case BadgeSymbol::Bass:   w += 22.0; break;
    case BadgeSymbol::Piano:  w += textFm.horizontalAdvance("p"); break;
    case BadgeSymbol::Forte:  w += textFm.horizontalAdvance("f"); break;
    }

    w += 2.0;
  }

  return w;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
void MorphSurfaceWidget::drawSymbols(QPainter& p, const QVector<BadgeSymbol>& parts, QPointF pos, qreal baseline, const QColor& color, const QFont& textFont) const
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
{
  p.save();
  p.setPen(color);

  for (BadgeSymbol part : parts)
  {
    switch (part)
    {
    case BadgeSymbol::Treble:
    {
      QPixmap pm = tintedIconPixmap(":/svg/treble-clef.svg", QSize(34, 34), color);
      p.drawPixmap(QPointF(pos.x(), baseline - 24.0), pm);
      pos.rx() += 28.0;
      break;
    }

    case BadgeSymbol::Bass:
    {
      QPixmap pm = tintedIconPixmap(":/svg/bass-clef.svg", QSize(22, 22), color);
      p.drawPixmap(QPointF(pos.x(), baseline - 18.0), pm);
      pos.rx() += 22.0;
      break;
    }

    case BadgeSymbol::Piano:
      p.setFont(textFont);
      p.drawText(QPointF(pos.x(), baseline), QStringLiteral("p"));
      pos.rx() += QFontMetricsF(textFont).horizontalAdvance("p");
      break;

    case BadgeSymbol::Forte:
      p.setFont(textFont);
      p.drawText(QPointF(pos.x(), baseline), QStringLiteral("f"));
      pos.rx() += QFontMetricsF(textFont).horizontalAdvance("f");
      break;
    }

    pos.rx() += 2.0;
  }

  p.restore();
}

//---------------------------------------------------------------
void MorphSurfaceWidget::onNoteOn(uint8_t note, uint8_t velocity)
//---------------------------------------------------------------
{
  note = std::clamp(note, minX_, maxX_);
  velocity = std::clamp(velocity, (uint8_t)0, (uint8_t)127);

  for (auto& n : currentNotes_)
  {
    if (n.key == note)
    {
      n.isOn = true;
      n.fading = false;
      n.alpha = 1.0f;
      n.fadeStartMs = 0;
      n.velocity = velocity;
      update();
      return;
    }
  }

  currentNotes_.push_back({ true, false, 1.0f, 0, note, velocity });
  update();
}


//----------------------------------------------
void MorphSurfaceWidget::onNoteOff(uint8_t note)
//----------------------------------------------
{
  for (auto& n : currentNotes_)
  {
    if (note == n.key)
    {
      // Se la nota era accesa, avvia il fade
      if (n.alpha > 0.0f)
      {
        n.isOn = false;
        n.fading = true;
        n.fadeStartMs = clock_.elapsed();

        startAnimationIfNeeded();
      }
      update();
      break;
    }
  }
}

//----------------------------------------
void MorphSurfaceWidget::onAnimationTick()
//----------------------------------------
{
  updateFadeStates();

  if (!hasActiveFades())
    animationTimer_.stop();

  update();
}

//-----------------------------------------
void MorphSurfaceWidget::updateFadeStates()
//-----------------------------------------
{
  const qint64 nowMs = clock_.elapsed();

  for (auto& n : currentNotes_)
  {
    if (!n.fading)
      continue;

    const qint64 elapsed = nowMs - n.fadeStartMs;
    float t = float(elapsed) / float(kFadeDurationMs); // 0..1
    t = std::clamp(t, 0.0f, 1.0f);
    float alpha = 1.0f - t * t; // decrescita un po' più naturale
    if (alpha <= 0.0f)
    {
      n.alpha = 0.0f;
      n.fading = false;
    }
    else
    {
      n.alpha = alpha;
    }
  }

  currentNotes_.remove_if([](const NoteDotState& n)
    {
      return !n.isOn && !n.fading;
    });
}

//---------------------------------------------
bool MorphSurfaceWidget::hasActiveFades() const
//---------------------------------------------
{
  for (auto& n : currentNotes_)
  {
    if (n.fading)
      return true;
  }
  return false;
}

//-----------------------------------------------
void MorphSurfaceWidget::startAnimationIfNeeded()
//-----------------------------------------------
{
  if (!animationTimer_.isActive())
    animationTimer_.start();
}

//------------------------------------------------
void MorphSurfaceWidget::stopAnimationIfPossible()
//------------------------------------------------
{
  if (!hasActiveFades())
    animationTimer_.stop();
}

//------------------------------------------------------------------------------------------------------------------------
void MorphSurfaceWidget::setActiveGroups(const std::array<std::list<uint8_t>, (uint8_t)TrackGroupId::Count>& activeGroups)
//------------------------------------------------------------------------------------------------------------------------
{
  activeGroups_ = activeGroups;
  update();
}

//---------------------------------------------------------
void MorphSurfaceWidget::paintEvent(QPaintEvent* /*event*/)
//---------------------------------------------------------
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setRenderHint(QPainter::TextAntialiasing, true);

  if (!image_.isNull())
    p.drawImage(rect(), image_);

  // Bordo
  p.setPen(QPen(QColor(40, 40, 40), 2));
  p.drawRect(rect().adjusted(1, 1, -1, -1));

  // Grid
  const int cy = height() / 2;

  p.setPen(QPen(QColor(255, 255, 255, 120), 1));
  p.drawLine(0, cy, width(), cy);

  const QVector<int> xMarks = visibleXMarks();
  for (int xMark : xMarks)
  {
    const int cx = width() * double(xMark - minX_) / double(maxX_ - minX_);
    p.drawLine(cx, 0, cx, height() - 20);
    QRect textRect(cx - 16, 2, 32, height() - 4);
    p.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, "C" + QString::number((xMark - 12) / 12));
  }

  for (auto& n : currentNotes_)
  {
    float alpha = 0.0f;
    if (n.isOn)
      alpha = 1.0f;
    else if (n.fading)
      alpha = n.alpha;
    else
      continue;

    QPointF center = dotPositionFor(n.key, n.velocity);

    QColor dotColor(n.fading ? QColor(200, 100, 100) : Qt::red);
    dotColor.setAlphaF(std::clamp(alpha, 0.0f, 1.0f));

    p.setPen(QPen(Qt::red, 1));
    p.setBrush(dotColor);

    constexpr qreal radius = 5.0;
    p.drawEllipse(center, radius, radius);
  }

  for (int g = 0; g < 8; ++g)
    drawBadge(p, g);

  drawExpandedBadgePanel(p);
}

//--------------------------------------------------------------------------
QPointF MorphSurfaceWidget::dotPositionFor(int midiNote, int velocity) const
//--------------------------------------------------------------------------
{
  const QRectF r = rect().adjusted(10, 10, -10, -10);

  const qreal x = r.left() + (r.width() * (midiNote - minX_)) / qreal(maxX_ - minX_);
  const qreal y = r.bottom() - (r.height() * velocity) / 127.0;

  return QPointF(x, y);
}

//-------------------------------------------------------------
bool MorphSurfaceWidget::handlePointerPress(const QPointF& pos)
//-------------------------------------------------------------
{
  if (expandedGroupIndex_ >= 0)
  {
    if (expandedGroupIndex_ < static_cast<int>(badgeRects_.size()) &&
        badgeRects_[expandedGroupIndex_].contains(pos))
    {
      closeExpandedBadgeAndNotify();
      return true;
    }

    if (expandedHeaderRect_.contains(pos))
    {
      closeExpandedBadgeAndNotify();
      return true;
    }

    for (int t = 0; t < 16; ++t)
    {
      if (expandedTrackRects_[t].contains(pos))
      {
        expandedTrackMask_ ^= uint16_t(1u << t);
        update();
        return true;
      }
    }

    if (!expandedPanelRect_.contains(pos))
    {
      closeExpandedBadgeAndNotify();

      // The same press may have happened on another badge: after closing,
      // treat that badge as the new target so switching group is immediate.
      for (int g = 0; g < static_cast<int>(TrackGroupId::None); ++g)
      {
        if (badgeRects_[g].contains(pos))
        {
          openExpandedBadge(uint8_t(g));
          return true;
        }
      }

      return true;
    }

    return true;
  }

  for (int g = 0; g < static_cast<int>(TrackGroupId::None); ++g)
  {
    if (badgeRects_[g].contains(pos))
    {
      openExpandedBadge(uint8_t(g));
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------
uint8_t MorphSurfaceWidget::noteFromSurfacePoint(const QPointF& pos) const
//------------------------------------------------------------------
{
  const QRectF r = rect().adjusted(10, 10, -10, -10);

  if (r.width() <= 0.0)
    return minX_;

  const qreal x = std::clamp(pos.x(), r.left(), r.right());
  const qreal normalized = (x - r.left()) / r.width();

  const int note = int(std::lround(double(minX_) + normalized * double(maxX_ - minX_)));
  return uint8_t(std::clamp(note, int(minX_), int(maxX_)));
}

//----------------------------------------------------------------------
uint8_t MorphSurfaceWidget::velocityFromSurfacePoint(const QPointF& pos) const
//----------------------------------------------------------------------
{
  const QRectF r = rect().adjusted(10, 10, -10, -10);

  if (r.height() <= 0.0)
    return 0;

  const qreal y = std::clamp(pos.y(), r.top(), r.bottom());
  const qreal normalized = (r.bottom() - y) / r.height();

  const int velocity = int(std::lround(normalized * 127.0));
  return uint8_t(std::clamp(velocity, 0, 127));
}

//----------------------------------------------------------------
void MorphSurfaceWidget::startSurfaceTestNote(const QPointF& pos)
//----------------------------------------------------------------
{
  const uint8_t note = noteFromSurfacePoint(pos);
  const uint8_t velocity = velocityFromSurfacePoint(pos);

  surfaceTestNoteActive_ = true;
  surfaceTestNote_ = note;
  surfaceTestVelocity_ = velocity;

  emit surfaceTestNoteOn(note, velocity);
}

//-----------------------------------------------------------------
void MorphSurfaceWidget::updateSurfaceTestNote(const QPointF& pos)
//-----------------------------------------------------------------
{
  if (!surfaceTestNoteActive_)
  {
    startSurfaceTestNote(pos);
    return;
  }

  const uint8_t note = noteFromSurfacePoint(pos);
  const uint8_t velocity = velocityFromSurfacePoint(pos);

  // A vertical move changes the Note On velocity and therefore must also
  // regenerate the test note: this is useful to audition the Piano/Forte
  // morphing axis even when the pitch does not change.
  if (note == surfaceTestNote_ && velocity == surfaceTestVelocity_)
    return;

  emit surfaceTestNoteOff(surfaceTestNote_);

  surfaceTestNote_ = note;
  surfaceTestVelocity_ = velocity;
  emit surfaceTestNoteOn(surfaceTestNote_, surfaceTestVelocity_);
}

//-------------------------------------------------
void MorphSurfaceWidget::stopSurfaceTestNote()
//-------------------------------------------------
{
  if (!surfaceTestNoteActive_)
    return;

  emit surfaceTestNoteOff(surfaceTestNote_);

  surfaceTestNoteActive_ = false;
  surfaceTestNote_ = 0;
  surfaceTestVelocity_ = 0;
}

//-------------------------------------------------------
void MorphSurfaceWidget::resizeEvent(QResizeEvent* event)
//-------------------------------------------------------
{
  QWidget::resizeEvent(event);
  rebuildImage();
  update();
}

//----------------------------------------------------------
void MorphSurfaceWidget::mousePressEvent(QMouseEvent* event)
//----------------------------------------------------------
{
  if (handlePointerPress(event->position()))
    return;

  if (event->button() == Qt::LeftButton)
  {
    startSurfaceTestNote(event->position());
    event->accept();
    return;
  }

  QWidget::mousePressEvent(event);
}

//---------------------------------------------------------
void MorphSurfaceWidget::mouseMoveEvent(QMouseEvent* event)
//---------------------------------------------------------
{
  if (surfaceTestNoteActive_ && (event->buttons() & Qt::LeftButton))
  {
    updateSurfaceTestNote(event->position());
    event->accept();
    return;
  }

  QWidget::mouseMoveEvent(event);
}

//------------------------------------------------------------
void MorphSurfaceWidget::mouseReleaseEvent(QMouseEvent* event)
//------------------------------------------------------------
{
  if (surfaceTestNoteActive_ && event->button() == Qt::LeftButton)
  {
    stopSurfaceTestNote();
    event->accept();
    return;
  }

  QWidget::mouseReleaseEvent(event);
}

//-------------------------------------------
bool MorphSurfaceWidget::event(QEvent* event)
//-------------------------------------------
{
  switch (event->type())
  {
  case QEvent::TouchBegin:
  {
    auto* touchEvent = static_cast<QTouchEvent*>(event);

    if (!touchEvent->points().isEmpty())
    {
      const QPointF pos = touchEvent->points().first().position();

      if (handlePointerPress(pos))
      {
        event->accept();
        return true;
      }

      startSurfaceTestNote(pos);
      event->accept();
      return true;
    }

    break;
  }

  case QEvent::TouchUpdate:
  {
    auto* touchEvent = static_cast<QTouchEvent*>(event);

    if (surfaceTestNoteActive_ && !touchEvent->points().isEmpty())
    {
      updateSurfaceTestNote(touchEvent->points().first().position());
      event->accept();
      return true;
    }

    break;
  }

  case QEvent::TouchEnd:
  case QEvent::TouchCancel:
  {
    if (surfaceTestNoteActive_)
    {
      stopSurfaceTestNote();
      event->accept();
      return true;
    }

    break;
  }

  default:
    break;
  }

  return QWidget::event(event);
}

//-------------------------------------
void MorphSurfaceWidget::rebuildImage()
//-------------------------------------
{
  if (width() <= 0 || height() <= 0)
  {
    image_ = QImage();
    return;
  }

  image_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);
  image_.fill(Qt::transparent);

  const int w = image_.width();
  const int h = image_.height();

  // Hue: da blu (240°) a giallo (60°)
  const int hueLeft = 240;
  const int hueRight = 60;

  for (int y = 0; y < h; ++y)
  {
    // Y controlla la luminosità
    const float ty = (h > 1) ? (1.0f - float(y) / float(h - 1)) : 0.0f;

    // basso scuro, alto chiaro
    const int value = int(80 + ty * (255 - 80));

    for (int x = 0; x < w; ++x)
    {
      // X controlla il colore
      const float tx = (w > 1) ? (float(x) / float(w - 1)) : 0.0f;

      const int hue = int(hueLeft + tx * (hueRight - hueLeft));

      // meno saturazione in alto
      const int saturation = int(200 - ty * 120);

      const QColor c = QColor::fromHsv(hue, saturation, value);

      image_.setPixelColor(x, y, c);
    }
  }
}

//--------------------------------------------------------------
QPoint MorphSurfaceWidget::badgeCenter(uint8_t groupIndex) const
//--------------------------------------------------------------
{
  const int w = width();
  const int h = height();

  const int marginX = 68;
  const int marginY = 34;

  const int left = marginX;
  const int right = w - marginX;
  const int top = marginY;
  const int bottom = h - marginY;
  const int cx = w / 2;
  const int cy = h / 2;

  switch (groupIndex)
  {
  case 0: return QPoint(cx, top);    // A = alto centro
  case 1: return QPoint(right, top);    // B = alto destra
  case 2: return QPoint(right, cy);     // C = destra centro
  case 3: return QPoint(right, bottom); // D = basso destra
  case 4: return QPoint(cx, bottom); // E = basso centro
  case 5: return QPoint(left, bottom); // F = basso sinistra
  case 6: return QPoint(left, cy);     // G = sinistra centro
  case 7: return QPoint(left, top);    // H = alto sinistra
  default:
    return QPoint(cx, cy);
  }
}

//-------------------------------------------------------------------
QColor MorphSurfaceWidget::sampleSurfaceColor(const QPoint& pt) const
//-------------------------------------------------------------------
{
  if (image_.isNull())
    return QColor(255, 255, 255);

  const int x = std::clamp(pt.x(), 0, image_.width() - 1);
  const int y = std::clamp(pt.y(), 0, image_.height() - 1);

  return image_.pixelColor(x, y);
}

//------------------------------------------------------------------
QString MorphSurfaceWidget::badgeIconLabel(uint8_t groupIndex) const
//------------------------------------------------------------------
{
  const QString family = musicFontFamily();

  if (!family.isEmpty())
  {
    // SMuFL / Bravura glyphs.
    // gClef        U+E050
    // fClef        U+E062
    // dynamicPiano U+E520
    // dynamicForte U+E522
    const QString treble = QString(QChar(0xE050));
    const QString bass   = QString(QChar(0xE062));
    const QString piano  = QString(QChar(0xE520));
    const QString forte  = QString(QChar(0xE522));

    switch (groupIndex)
    {
    case 0: return forte;           // top
    case 1: return treble + forte;  // top right
    case 2: return treble;          // right
    case 3: return treble + piano;  // bottom right
    case 4: return piano;           // bottom
    case 5: return bass + piano;    // bottom left
    case 6: return bass;            // left
    case 7: return bass + forte;    // top left
    default: return QString();
    }
  }

  // Fallback without SMuFL font: standard Unicode clefs plus italic letters.
  // It is less elegant than Bravura, but keeps the widget readable.
  const QString treble = ucs4(0x1D11E); // MUSICAL SYMBOL G CLEF
  const QString bass   = ucs4(0x1D122); // MUSICAL SYMBOL F CLEF
  const QString piano  = QStringLiteral("p");
  const QString forte  = QStringLiteral("f");

  switch (groupIndex)
  {
  case 0: return forte;
  case 1: return treble + " " + forte;
  case 2: return treble;
  case 3: return treble + " " + piano;
  case 4: return piano;
  case 5: return bass + " " + piano;
  case 6: return bass;
  case 7: return bass + " " + forte;
  default: return QString();
  }
}


//--------------------------------------------------------------------
QString MorphSurfaceWidget::badgeTrackList(uint8_t groupIndex) const
//--------------------------------------------------------------------
{
  if (groupIndex >= activeGroups_.size())
    return QString();

  QStringList parts;

  for (uint8_t trackIndex : activeGroups_[groupIndex])
    parts << QString::number(int(trackIndex) + 1); // trackIndex is internally 0-based

  return parts.join(QStringLiteral(", "));
}


//--------------------------------------------------------------
uint16_t MorphSurfaceWidget::trackMaskForGroup(uint8_t groupIndex) const
//--------------------------------------------------------------
{
  if (groupIndex >= activeGroups_.size())
    return 0;

  uint16_t mask = 0;
  for (uint8_t trackIndex : activeGroups_[groupIndex])
  {
    if (trackIndex < 16)
      mask |= uint16_t(1u << trackIndex);
  }

  return mask;
}

//--------------------------------------------------------
void MorphSurfaceWidget::openExpandedBadge(uint8_t groupIndex)
//--------------------------------------------------------
{
  if (groupIndex >= static_cast<uint8_t>(TrackGroupId::None))
    return;

  expandedGroupIndex_ = int(groupIndex);
  expandedTrackMask_ = trackMaskForGroup(groupIndex);
  update();
}

//--------------------------------------------------
void MorphSurfaceWidget::closeExpandedBadgeAndNotify()
//--------------------------------------------------
{
  if (expandedGroupIndex_ < 0)
    return;

  const TrackGroupId group = static_cast<TrackGroupId>(expandedGroupIndex_);
  const uint16_t mask = expandedTrackMask_;

  expandedGroupIndex_ = -1;
  expandedTrackMask_ = 0;
  clearExpandedBadgeGeometry();

  update();
  emit groupBadgeTrackMaskEdited(group, mask);
}

//---------------------------------------------------
void MorphSurfaceWidget::clearExpandedBadgeGeometry()
//---------------------------------------------------
{
  expandedPanelRect_ = QRectF();
  expandedHeaderRect_ = QRectF();
  for (auto& r : expandedTrackRects_)
    r = QRectF();
}

//----------------------------------------------------
QVector<int> MorphSurfaceWidget::visibleXMarks() const
//----------------------------------------------------
{
  QVector<int> marks;

  static constexpr int baseMarks[] = { 12, 24, 36, 48, 60, 72, 84, 96, 108, 120 };
  for (int v : baseMarks)
    if (v >= minX_ && v <= maxX_)
      marks.push_back(v);

  std::sort(marks.begin(), marks.end());
  return marks;
}

//----------------------------------------------------------
void MorphSurfaceWidget::drawExpandedBadgePanel(QPainter& p)
//----------------------------------------------------------
{
  if (expandedGroupIndex_ < 0 ||
      expandedGroupIndex_ >= static_cast<int>(TrackGroupId::None))
  {
    clearExpandedBadgeGeometry();
    return;
  }

  const uint8_t groupIndex = uint8_t(expandedGroupIndex_);

  const auto symbols = badgeSymbols(groupIndex);
  if (symbols.isEmpty())
    return;

  QFont titleFont = p.font();
  titleFont.setBold(true);
  titleFont.setPixelSize(14);

  QFont itemFont = p.font();
  itemFont.setPixelSize(13);

  QFont dynamicFont = titleFont;
  dynamicFont.setItalic(true);
  dynamicFont.setBold(true);
  dynamicFont.setPixelSize(22);

  constexpr qreal kOuterMargin = 8.0;
  constexpr qreal kPadding = 10.0;
  constexpr qreal kHeaderH = 38.0;
  constexpr qreal kRowH = 34.0;
  constexpr qreal kColumnW = 112.0;

  const bool useTwoColumns = height() < 560;
  const int columnCount = useTwoColumns ? 2 : 1;
  const int rowCount = useTwoColumns ? 8 : 16;

  const qreal panelW = columnCount * kColumnW + 2.0 * kPadding;
  const qreal panelH = kHeaderH + rowCount * kRowH + 2.0 * kPadding;

  const QPoint c = badgeCenter(groupIndex);

  qreal left = qreal(c.x()) - panelW / 2.0;
  qreal top = qreal(c.y()) - panelH / 2.0;

  if (groupIndex == 0)
    top = badgeRects_[groupIndex].bottom() + 4.0;
  else if (groupIndex == 4)
    top = badgeRects_[groupIndex].top() - panelH - 4.0;
  else if (groupIndex == 1 || groupIndex == 2 || groupIndex == 3)
    left = badgeRects_[groupIndex].left() - panelW - 4.0;
  else if (groupIndex == 5 || groupIndex == 6 || groupIndex == 7)
    left = badgeRects_[groupIndex].right() + 4.0;

  left = std::clamp(left, kOuterMargin, std::max(kOuterMargin, qreal(width()) - panelW - kOuterMargin));
  top = std::clamp(top, kOuterMargin, std::max(kOuterMargin, qreal(height()) - panelH - kOuterMargin));

  QRectF panelRect(left, top, panelW, panelH);
  expandedPanelRect_ = panelRect;

  QColor fill = sampleSurfaceColor(QPoint(int(panelRect.center().x()), int(panelRect.center().y()))).lighter(125);

  const int luminance =
    int(0.299 * fill.red() +
        0.587 * fill.green() +
        0.114 * fill.blue());

  const QColor textColor = (luminance < 140)
    ? QColor(255, 255, 255)
    : QColor(20, 20, 20);

  const QColor borderColor = (luminance < 140)
    ? QColor(255, 255, 255, 150)
    : QColor(40, 40, 40, 190);

  QColor shadow(0, 0, 0, 80);
  p.setPen(Qt::NoPen);
  p.setBrush(shadow);
  p.drawRoundedRect(panelRect.translated(2.0, 3.0), 8.0, 8.0);

  p.setPen(QPen(borderColor, 1));
  p.setBrush(fill);
  p.drawRoundedRect(panelRect, 8.0, 8.0);

  const QRectF headerRect(panelRect.left() + kPadding,
                          panelRect.top() + kPadding,
                          panelRect.width() - 2.0 * kPadding,
                          kHeaderH - 4.0);
  expandedHeaderRect_ = headerRect;

  const qreal iconW = badgeSymbolWidth(symbols, QFontMetricsF(titleFont));
  const QString titleText = QStringLiteral(" : tracks");
  const qreal titleW = QFontMetricsF(titleFont).horizontalAdvance(titleText);
  const qreal headerTextW = iconW + titleW;
  qreal x = headerRect.center().x() - headerTextW / 2.0;
  const qreal baseline = headerRect.top() + 24.0;

  p.save();
  p.setPen(textColor);

  drawSymbols(p, symbols, QPointF(x, baseline), baseline, textColor, dynamicFont);
  x += iconW;

  p.setFont(titleFont);
  p.drawText(QPointF(x, baseline), titleText);

  p.setPen(QPen(borderColor, 1));
  const qreal lineY = panelRect.top() + kPadding + kHeaderH - 4.0;
  p.drawLine(QPointF(panelRect.left() + kPadding, lineY),
             QPointF(panelRect.right() - kPadding, lineY));

  QFontMetricsF itemFm(itemFont);
  p.setFont(itemFont);

  for (int t = 0; t < 16; ++t)
  {
    const int col = useTwoColumns ? (t / rowCount) : 0;
    const int row = useTwoColumns ? (t % rowCount) : t;

    QRectF r(panelRect.left() + kPadding + col * kColumnW,
             panelRect.top() + kPadding + kHeaderH + row * kRowH,
             kColumnW,
             kRowH);

    expandedTrackRects_[t] = r;

    const bool checked = (expandedTrackMask_ & uint16_t(1u << t)) != 0;

    QColor rowFill = checked
      ? QColor(textColor.red(), textColor.green(), textColor.blue(), luminance < 140 ? 45 : 28)
      : QColor(0, 0, 0, 0);

    p.setPen(Qt::NoPen);
    p.setBrush(rowFill);
    p.drawRoundedRect(r.adjusted(2.0, 3.0, -2.0, -3.0), 5.0, 5.0);

    p.setPen(textColor);
    const QString checkText = checked ? QStringLiteral("✓") : QStringLiteral(" ");
    const QString label = QStringLiteral("%1 Track %2").arg(checkText).arg(t + 1);

    const qreal itemBaseline = r.center().y() + (itemFm.ascent() - itemFm.descent()) / 2.0;
    p.drawText(QPointF(r.left() + 8.0, itemBaseline), label);
  }

  p.restore();
}


//-----------------------------------------------------------------
void MorphSurfaceWidget::drawBadge(QPainter& p, uint8_t groupIndex)
//-----------------------------------------------------------------
{
  const QPoint c = badgeCenter(groupIndex);

  const auto symbols = badgeSymbols(groupIndex);
  if (symbols.isEmpty())
    return;

  const QString tracksText = badgeTrackList(groupIndex);


  //QFont iconFont;
  //const QString family = musicFontFamily();
  //if (!family.isEmpty())
  //{
  //  iconFont = QFont(family);
  //  iconFont.setPixelSize(iconText.size() > 1 ? 24 : 27);
  //}
  //else
  //{
  //  iconFont = p.font();
  //  iconFont.setItalic(true);
  //  iconFont.setBold(true);
  //  iconFont.setPixelSize(iconText.size() > 1 ? 22 : 25);
  //}

  QFont textFont = p.font();
  QFont dynamicFont = textFont;
  dynamicFont.setItalic(true);
  dynamicFont.setBold(true);
  dynamicFont.setPixelSize(22);
  //QFontMetricsF iconFm(iconFont);
  QFontMetricsF textFm(textFont);

  const QString expandText = (expandedGroupIndex_ == int(groupIndex))
    ? QStringLiteral("▲ ")
    : QStringLiteral("▼ ");
  const QString separator = QStringLiteral(" : ");

  const qreal expandW = textFm.horizontalAdvance(expandText);
  const qreal iconW = badgeSymbolWidth(symbols, textFm);
  const qreal sepW = textFm.horizontalAdvance(separator);
  const qreal tracksW = textFm.horizontalAdvance(tracksText);
  const qreal totalTextW = expandW + iconW + sepW + tracksW;

  // The old badge was fixed-size because it only contained a symbol.
  // Now it also contains the assigned tracks, so the rectangle must be
  // sized from the actual text and then clamped inside the widget.
  constexpr qreal kPaddingX = 12.0;
  constexpr qreal kPaddingY = 7.0;
  constexpr qreal kMinBadgeW = 118.0;
  constexpr qreal kBadgeH = 42.0;
  constexpr qreal kOuterMargin = 8.0;

  const qreal maxBadgeW = std::max<qreal>(kMinBadgeW,
    qreal(width()) - 2.0 * kOuterMargin);

  qreal badgeW = std::max(kMinBadgeW, totalTextW + 2.0 * kPaddingX);
  badgeW = std::min(badgeW, maxBadgeW);

  qreal left = c.x() - badgeW / 2.0;
  left = std::clamp(left, kOuterMargin, qreal(width()) - badgeW - kOuterMargin);

  QRectF r(left,
          c.y() - kBadgeH / 2.0,
          badgeW,
          kBadgeH);

  // Vertical clamp, useful if the widget becomes very small.
  if (r.top() < kOuterMargin)
    r.moveTop(kOuterMargin);
  if (r.bottom() > height() - kOuterMargin)
    r.moveBottom(height() - kOuterMargin);

  QColor fill = sampleSurfaceColor(c);

  // Leggera schiarita per rendere il badge un po' più "widget"
  fill = fill.lighter(120);

  // Luminanza percettiva
  const int luminance =
    int(0.299 * fill.red() +
        0.587 * fill.green() +
        0.114 * fill.blue());

  const QColor textColor = (luminance < 140)
    ? QColor(255, 255, 255)
    : QColor(20, 20, 20);

  const QColor borderColor = (luminance < 140)
    ? QColor(255, 255, 255, 120)
    : QColor(40, 40, 40, 180);

  p.setPen(QPen(borderColor, 1));
  p.setBrush(fill);
  p.drawRoundedRect(r, 7.0, 7.0);

  const qreal availableTextW = r.width() - 2.0 * kPaddingX;
  const qreal maxTracksW = std::max<qreal>(0.0, availableTextW - expandW - iconW - sepW);

  QString visibleTracks = tracksText;
  if (tracksW > maxTracksW)
    visibleTracks = textFm.elidedText(tracksText, Qt::ElideRight, int(maxTracksW));

  const qreal visibleTracksW = textFm.horizontalAdvance(visibleTracks);
  const qreal visibleTotalW = expandW + iconW + sepW + visibleTracksW;

  // Baseline comune: il testo normale governa la centratura verticale;
  // il simbolo musicale viene appoggiato alla stessa baseline.
  const qreal baseline = r.center().y() + (textFm.ascent() - textFm.descent()) / 2.0;
  qreal x = r.center().x() - visibleTotalW / 2.0;

  p.save();
  p.setPen(textColor);

  p.setFont(textFont);
  p.drawText(QPointF(x, baseline), expandText);
  x += expandW;

  drawSymbols(p, symbols, QPointF(x, baseline), baseline, textColor, dynamicFont);
  x += iconW;

  p.setFont(textFont);
  p.drawText(QPointF(x, baseline), separator);
  x += sepW;

  p.drawText(QPointF(x, baseline), visibleTracks);

  p.restore();

  badgeRects_[groupIndex] = r;
}
