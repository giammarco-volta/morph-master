#include "MainWindow.h"

#include <QTabWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QTextBrowser>
#include <QDesktopServices>
#include <QUrl>
#include <QElapsedTimer>
#include <QtConcurrent>

#include "ui/tabs/MidiSettingsTab.h"
#include "ui/tabs/SurfaceTab.h"
#include "ui/tabs/FourTracksTab.h"  
#include "ui/tabs/CurveEditorTab.h"
#include "ui/widgets/TrackStripWidget.h"
#include "ui/widgets/MorphSurfaceWidget.h"
#include "core/ExpressionCurveId.h"
#include "core/ExpressionCalculator.h"
#include "core/About.h"

#include "../../Common/src/ui/widgets/ManualWidget.h"

#ifdef Q_OS_ANDROID

#include <QJniObject>

static void enableKeepScreenOn()
{
  QJniObject activity =
    QJniObject::callStaticObjectMethod(
      "org/qtproject/qt/android/QtNative",
      "activity",
      "()Landroid/app/Activity;");

  if (!activity.isValid())
    return;

  QJniObject window =
    activity.callObjectMethod(
      "getWindow",
      "()Landroid/view/Window;");

  if (!window.isValid())
    return;

  constexpr int FLAG_KEEP_SCREEN_ON = 128;

  window.callMethod<void>(
    "addFlags",
    "(I)V",
    FLAG_KEEP_SCREEN_ON);
}
#endif

//-----------------------------------------------------------
static void sendRpnFine(IMidiOut& out, uint8_t ch, int cents)
//-----------------------------------------------------------
{
  // RPN Coarse Tuning: RPN 0,1 ; Data Entry MSB = 64 + cents
  int v = 64 + cents;
  if (v < 0)
    v = 0;
  if (v > 127)
    v = 127;

  out.sendShort(0xB0 | (ch & 0x0F), 101, 0);     // CC101 RPN MSB
  out.sendShort(0xB0 | (ch & 0x0F), 100, 1);     // CC100 RPN LSB
  out.sendShort(0xB0 | (ch & 0x0F), 6, (uint8_t)v); // CC6 Data Entry MSB
  out.sendShort(0xB0 | (ch & 0x0F), 101, 127);   // deselect
  out.sendShort(0xB0 | (ch & 0x0F), 100, 127);
}

//-----------------------------------------------------------------------
static void sendProgramChange(IMidiOut& out, uint8_t ch, uint8_t program)
//-----------------------------------------------------------------------
{
  static constexpr uint8_t dummyData2 = 0;
  out.sendShort(0xC0 | (ch & 0x0F), program, dummyData2);
}

//---------------------------------------------------------------------------------
static void sendControlChange(IMidiOut& out, uint8_t ch, uint8_t cc, uint8_t value)
//---------------------------------------------------------------------------------
{
  out.sendShort(0xB0 | (ch & 0x0F), cc, value);
}

//------------------------------------------------------------------------------
static void sendNoteOn(IMidiOut& out, uint8_t ch, uint8_t key, uint8_t velocity)
//------------------------------------------------------------------------------
{
  out.sendShort(0x90 | (ch & 0x0F), key, velocity);
}

//-------------------------------------------------------------------------------
static void sendNoteOff(IMidiOut& out, uint8_t ch, uint8_t key, uint8_t velocity)
//-------------------------------------------------------------------------------
{
  out.sendShort(0x80 | (ch & 0x0F), key, velocity);
}

//-----------------------------------------------------------------------------------------------
static void sendChannelMsg(IMidiOut& out, uint8_t ch, uint8_t code, uint8_t data1, uint8_t data2)
//-----------------------------------------------------------------------------------------------
{
  out.sendShort(code | (ch & 0x0F), data1, data2);
}

//----------------------------------------------------
static void sendAllNotesOff(IMidiOut& out, uint8_t ch)
//----------------------------------------------------
{
  out.sendShort(0xB0 | (ch & 0x0F), 123, 0);
}

static constexpr const char* ksMono = "mono";
static constexpr const char* ksMonoRetrigOrigVel = "monoRetrigOrigVel";
static constexpr const char* ksMonoRetrigVelOff = "monoRetrigVelOff";
static constexpr const char* ksPoly = "poly";
static constexpr const char* ksManual = "manual";
static constexpr const char* ksInstrument = "instrument";

//-------------------------------------------------
QString MainWindow::playModeToString(PlayMode mode)
//-------------------------------------------------
{
  switch (mode) {
  case PlayMode::MonoNoRetrig:      return ksMono;
  case PlayMode::MonoRetrigOrigVel: return ksMonoRetrigOrigVel;
  case PlayMode::MonoRetrigVelOff:  return ksMonoRetrigVelOff;
  case PlayMode::Poly:              return ksPoly;
  }
  return ksPoly;
}

//-------------------------------------------------------
QString MainWindow::patchPolicyToString(PatchPolicy mode)
//-------------------------------------------------------
{
  switch (mode) {
  case PatchPolicy::Manual:     return ksManual;
  case PatchPolicy::Instrument: return ksInstrument;
  }
  return ksManual;
}

//-------------------------------------------------------
PlayMode MainWindow::playModeFromString(const QString& s)
//------------------------------------------------------
{
  if (s == ksMono)              return PlayMode::MonoNoRetrig;
  if (s == ksMonoRetrigOrigVel) return PlayMode::MonoRetrigOrigVel;
  if (s == ksMonoRetrigVelOff)  return PlayMode::MonoRetrigVelOff;
  if (s == ksPoly)              return PlayMode::Poly;
  return PlayMode::Poly;
}

//-------------------------------------------------------------
PatchPolicy MainWindow::patchPolicyFromString(const QString& s)
//------------------------------------------------------ ------
{
  if (s == ksManual)     return PatchPolicy::Manual;
  if (s == ksInstrument) return PatchPolicy::Instrument;
  return PatchPolicy::Manual;
}

//--------------------------------------------------
AppInitSettings MainWindow::loadInitSettings() const
//--------------------------------------------------
{
  QSettings settings("NaadaLab", "MorphMaster");

  AppInitSettings init;
  settings.beginGroup("midi");
  init.midiSetup.midiOutPort = settings.value("outPortName").toString();
  init.midiSetup.midiInPort = settings.value("inPortName").toString();
  init.midiSetup .midiInChannel = settings.value("midiInChn", 0).toUInt();
  settings.endGroup();

  settings.beginGroup("play");
  init.playMode = playModeFromString(settings.value("playMode", "poly").toString());
  settings.endGroup();

  settings.beginGroup("settings");
  init.patchPolicy = patchPolicyFromString(settings.value("patchPolicy", "manual").toString());
  init.knownInstrumentName = settings.value("selectedInstrument", "").toString();
  init.keyboardRangeId = static_cast<KeyboardRangeId>(settings.value("keyboardRange", 0).toInt());
  settings.endGroup();

  return init;
}

//---------------------------------------
void MainWindow::saveInitSettings() const
//---------------------------------------
{
  if (midiSettingTab_ == nullptr)
    return;

  if (loadingPreset_)
    return;

  QSettings settings("NaadaLab", "MorphMaster");

  AppInitSettings preset;
  midiSettingTab_->getPresetData(preset);

  settings.beginGroup("midi");
  settings.setValue("outPortName", preset.midiSetup.midiOutPort);
  settings.setValue("inPortName", preset.midiSetup.midiInPort);
  settings.setValue("midiInChn", preset.midiSetup.midiInChannel);
  settings.endGroup();

  settings.beginGroup("play");
  settings.setValue("playMode", playModeToString(preset.playMode));
  settings.endGroup();

  settings.beginGroup("settings");
  settings.setValue("patchPolicy", patchPolicyToString(preset.patchPolicy));
  settings.setValue("selectedInstrument", preset.knownInstrumentName);
  settings.setValue("keyboardRange", static_cast<int>(preset.keyboardRangeId));
  settings.endGroup();

  settings.sync();
}

//--------------------------------------------------------------------------
std::list<uint8_t> MainWindow::getTracksForGroup(TrackGroupId groupId) const
//--------------------------------------------------------------------------
{
  std::list<uint8_t> tracks;

  for (auto* fourTracksTab : fourTracksTabs_)
    for (int i = 0; i < 4; ++i)
      if (fourTracksTab->getGroupIdByRelTrack(i) == groupId)
        tracks.push_back(fourTracksTab->trackIndex(i));

  return tracks;
}

//-----------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
//-----------------------------------------------------------
{
#ifdef Q_OS_ANDROID
  QJniObject::callStaticMethod<void>(
    "org/qtproject/qt/android/MidiAndroidBridge",
    "keepScreenOn",
    "()V");
#endif

  QElapsedTimer t;
  t.start();

  qDebug() << "BOOT 00 MainWindow ctor begin";

  setWindowTitle("MorphMaster");

  auto* mainTab = new QTabWidget(this);
  auto* settingsTab = new QTabWidget(mainTab);
  auto* tracksTab = new QTabWidget(settingsTab);
  auto* curvesTab = new QTabWidget(mainTab);
  auto* helpTab = new QTabWidget(mainTab);

  for (int i = 0; i < 4; ++i)
  {
    fourTracksTabs_[i] = new FourTracksTab(this, i * 4);
    fourTracksTabs_[i]->setPatchPolicy(patchPolicy_);
    fourTracksTabs_[i]->setInstrumentDefinition(&instrumentDefinition_);
    qDebug() << "BOOT 01 fourTracksTabs_" << t.elapsed() << "ms";
  }

  midiSettingTab_ = new MidiSettingsTab(this);
  surfaceTab_ = new SurfaceTab(this);
  for (int i = 0; i < 2; ++i)
    curveEditorTab_[i] = new CurveEditorTab(this, i == 0);

  qDebug() << "BOOT 02 fourTracksTabs_" << t.elapsed() << "ms";

  auto* userManualWidget = new UserManualWidget(":/manual/MorphMasterUserManual.pdf", helpTab);
  helpTab->addTab(userManualWidget, tr("Manual"));

  qDebug() << "BOOT 03 userManualWidget" << t.elapsed() << "ms";

  auto* aboutTab = new QWidget(helpTab);
  auto* aboutLayout = new QVBoxLayout(aboutTab);

  auto* aboutText = new QTextBrowser(aboutTab);
  aboutText->setFrameShape(QFrame::StyledPanel);
  aboutText->setHtml(tr(about));

  qDebug() << "BOOT 04 about" << t.elapsed() << "ms";

  // Importante: disabilita l'apertura automatica dei link.
  // Così possiamo intercettarli noi.
  aboutText->setOpenLinks(false);
  aboutText->setOpenExternalLinks(false);

  connect(aboutText, &QTextBrowser::anchorClicked,
    this,
    [helpTab, userManualWidget](const QUrl& url)
    {
      if (url.toString() == "morphmaster:user-manual")
      {
        helpTab->setCurrentWidget(userManualWidget);
      }
      else
      {
        QDesktopServices::openUrl(url);
      }
    });

  aboutLayout->addWidget(aboutText);

  helpTab->addTab(aboutTab, tr("About"));

  tracksTab->addTab(fourTracksTabs_[0], tr("1-4"));
  tracksTab->addTab(fourTracksTabs_[1], tr("5-8"));
  tracksTab->addTab(fourTracksTabs_[2], tr("9-12"));
  tracksTab->addTab(fourTracksTabs_[3], tr("13-16"));

  settingsTab->addTab(midiSettingTab_, tr("General"));
  settingsTab->addTab(tracksTab, tr("Tracks"));
  settingsTab->addTab(curvesTab, tr("Curves"));

  curvesTab->addTab(curveEditorTab_[0], tr("Note morph curves"));
  curvesTab->addTab(curveEditorTab_[1], tr("Velocity morph curves"));

  mainTab->addTab(surfaceTab_, tr("Surface"));
  mainTab->addTab(settingsTab, tr("Settings"));
  mainTab->addTab(helpTab, tr("Help"));

  qDebug() << "BOOT 05 addTabs" << t.elapsed() << "ms";

  connect(midiSettingTab_, &MidiSettingsTab::midiNoteOnReceived, this, &MainWindow::onMidiNoteOnReceived);
  connect(midiSettingTab_, &MidiSettingsTab::midiNoteOffReceived, this, &MainWindow::onMidiNoteOffReceived);
  connect(midiSettingTab_, &MidiSettingsTab::midiChannelMsgReceived, this, &MainWindow::onMidiChannelMsgReceived);
  connect(midiSettingTab_, &MidiSettingsTab::instrumentDefinitionModeChanged, this, &MainWindow::onInstrumentDefinitionModeChanged);
  connect(midiSettingTab_, &MidiSettingsTab::knownInstrumentChanged,  this, &MainWindow::onKnownInstrumentChanged);

  presetManager_ = new PresetManager(this);

  connect(presetManager_, &PresetManager::presetsChanged, this, &MainWindow::refreshPresetUi);

  presetManager_->loadFromDisk();

  qDebug() << "BOOT 06 presetManager_->loadFromDisk()" << t.elapsed() << "ms";

  setCentralWidget(mainTab);
  resize(1200, 700);

  startInstrumentDatabaseLoading();

  qDebug() << "BOOT 07 loadInstrumentDatabase()" << t.elapsed() << "ms";

  connect(fourTracksTabs_[0]->trackWidget( 0), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[0]->trackWidget( 1), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[0]->trackWidget( 2), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[0]->trackWidget( 3), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[1]->trackWidget( 4), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[1]->trackWidget( 5), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[1]->trackWidget( 6), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[1]->trackWidget( 7), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[2]->trackWidget( 8), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[2]->trackWidget( 9), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[2]->trackWidget(10), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[2]->trackWidget(11), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[3]->trackWidget(12), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[3]->trackWidget(13), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[3]->trackWidget(14), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);
  connect(fourTracksTabs_[3]->trackWidget(15), &TrackStripWidget::trackGroupChanged, this, &MainWindow::onTrackGroupChanged);

  connect(surfaceTab_->getMorphSurfaceWidget(), &MorphSurfaceWidget::groupBadgeTrackMaskEdited, this, &MainWindow::onGroupBadgeTrackMaskEdited);
  connect(surfaceTab_->getMorphSurfaceWidget(), &MorphSurfaceWidget::surfaceTestNoteOn, this, &MainWindow::onSurfaceTestNoteOn);
  connect(surfaceTab_->getMorphSurfaceWidget(), &MorphSurfaceWidget::surfaceTestNoteOff, this, &MainWindow::onSurfaceTestNoteOff);

  connect(midiSettingTab_, &MidiSettingsTab::signalKeyboardRangeChanged, this, &MainWindow::onKeyboardRangeChanged);

  loadingPreset_ = true;
  AppInitSettings settings = loadInitSettings();
  pendingInstrumentName_ = settings.knownInstrumentName;
  midiSettingTab_->setFromPreset(settings);
  loadingPreset_ = false;

  qDebug() << "BOOT 08 loadInitSettings()" << t.elapsed() << "ms";

//#ifdef Q_OS_ANDROID
//  enableKeepScreenOn();
//#endif
}

//----------------------------------------------------------------------------------
void MainWindow::onMidiNoteOnReceived(uint8_t note, uint8_t velocity, IMidiOut* out)
//----------------------------------------------------------------------------------
{
  //qDebug() << "MORPHMASTER MAIN WINDOW MIDI: NOTE ON RECEIVED" << "note" << note << "velocity" << velocity;
  if (playMode_ == PlayMode::Poly)
  {
    surfaceTab_->onNoteOn(note, velocity);
    handleIncomingNote(note, velocity, out);
    return;
  }

  // Mono
  if (monoPlayingNote_.has_value())
  {
    const HeldNote playing = monoPlayingNote_.value();

    surfaceTab_->onNoteOff(playing.note);
    handleIncomingNoteOff(playing.note, 0, out);
  }

  currNotes_.remove_if([note](const HeldNote& hn)
    {
      return hn.note == note;
    });

  HeldNote newNote{ note, velocity };
  currNotes_.push_back(newNote);

  surfaceTab_->onNoteOn(note, velocity);
  handleIncomingNote(note, velocity, out);

  monoPlayingNote_ = newNote;
}

//-----------------------------------------------------------------------------------
void MainWindow::onMidiNoteOffReceived(uint8_t note, uint8_t velocity, IMidiOut* out)
//-----------------------------------------------------------------------------------
{
  //qDebug() << "MORPHMASTER MAIN WINDOW MIDI: NOTE OFF RECEIVED" << "note" << note << "velocity" << velocity;
  if (playMode_ == PlayMode::Poly)
  {
    surfaceTab_->onNoteOff(note);
    handleIncomingNoteOff(note, velocity, out);
    return;
  }

  // Mono
  currNotes_.remove_if([note](const HeldNote& hn)
    {
      return hn.note == note;
    });

  if (!monoPlayingNote_.has_value() || monoPlayingNote_->note != note)
    return;

  surfaceTab_->onNoteOff(note);
  handleIncomingNoteOff(note, velocity, out);

  if (!currNotes_.empty() && ((playMode_ == PlayMode::MonoRetrigOrigVel) || (playMode_ == PlayMode::MonoRetrigVelOff)))
  {
    const HeldNote next = currNotes_.back();

    uint8_t newNoteVelocity = (playMode_ == PlayMode::MonoRetrigVelOff) ? velocity : next.velocity;
    surfaceTab_->onNoteOn(next.note, newNoteVelocity);
    handleIncomingNote(next.note, newNoteVelocity, out);

    monoPlayingNote_ = next;
  }
  else
  {
    monoPlayingNote_.reset();
  }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::onMidiChannelMsgReceived(uint8_t code, uint8_t data1, uint8_t data2, IMidiOut* out)
//--------------------------------------------------------------------------------------------------
{
  handleIncomingChannelMsg(code, data1, data2, out);
}

//---------------------------------------------------------------------
void MainWindow::onInstrumentDefinitionModeChanged(bool instrumentMode)
//---------------------------------------------------------------------
{
  // salva stato globale se serve
  patchPolicy_ = instrumentMode ? PatchPolicy::Instrument : PatchPolicy::Manual;

  // aggiorna tutte le track strip
  for (auto* fourTracksTab : fourTracksTabs_)
    if (fourTracksTab)
      fourTracksTab->setPatchPolicy(patchPolicy_);

  saveInitSettings();
}

//----------------------------------------------------------------------
void MainWindow::onKnownInstrumentChanged(const QString& instrumentName)
//----------------------------------------------------------------------
{
  const InstrumentDefinition* def = instrumentDatabase_->findByName(instrumentName);
  if (!def)
    return;

  // aggiorna tutte le track strip
  for (auto* fourTracksTab : fourTracksTabs_)
    fourTracksTab->setInstrumentDefinition(def);

  saveInitSettings();
}

//----------------------------------------------------
void MainWindow::onKeyboardRangeChanged(uint8_t index)
//----------------------------------------------------
{
  const KeyboardRange& r = kKeyboardRanges[index];
  surfaceTab_->getMorphSurfaceWidget()->setXRange(r.minNote, r.maxNote);
  curveEditorTab_[0]->setKeyboardRange(static_cast<KeyboardRangeId>(index));

  saveInitSettings();
}

//--------------------------------------------------------------------------
void MainWindow::onTrackGroupChanged(uint8_t trackIdx, TrackGroupId groupId)
//--------------------------------------------------------------------------
{
  std::array<std::list<uint8_t>, (uint8_t)TrackGroupId::Count> activeGroups;

  for (int g = 0; g < static_cast<int>(TrackGroupId::Count); ++g)
    activeGroups[g] = getTracksForGroup(static_cast<TrackGroupId>(g));

  surfaceTab_->getMorphSurfaceWidget()->setActiveGroups(activeGroups);
}

//----------------------------------------------------------------------
void MainWindow::onTrackActivationChanged(uint8_t trackIdx, bool active)
//----------------------------------------------------------------------
{
  if (!active && activeTracks_.test(trackIdx))
  {
    // Se la traccia è stata disattivata, mando un all notes off per sicurezza
    IMidiOut& out = midiSettingTab_->MidiOut();
    sendAllNotesOff(out, trackIdx);
    sendControlChange(out, trackIdx, 11, 0x7F);
  }
  activeTracks_.set(trackIdx, active);
}

//---------------------------------------------------------------------------------------
void MainWindow::onGroupBadgeTrackMaskEdited(TrackGroupId groupIndex, uint16_t trackMask)
//---------------------------------------------------------------------------------------
{
  uint16_t prevMask = 0;
  for (uint8_t trk = 0; trk < 16; trk++)
    if (fourTracksTabs_[trk / 4]->getGroupIdByAbsTrack(trk) == groupIndex)
      prevMask |= (1 << trk);


  for (uint8_t trk = 0; trk < 16; trk++)
  {
    if ((prevMask & (1 << trk)) && !(trackMask & (1 << trk)))
      fourTracksTabs_[trk/4]->removeTrackFromGroup(trk, groupIndex);
    else if (!(prevMask & (1 << trk)) && (trackMask & (1 << trk)))
      fourTracksTabs_[trk / 4]->assignTrackToGroup(trk, groupIndex);
  }

  onTrackGroupChanged(0, groupIndex);
}

//------------------------------------------------------------------
void MainWindow::onSurfaceTestNoteOn(uint8_t note, uint8_t velocity)
//------------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  onMidiNoteOnReceived(note, velocity, &out);
}

//-------------------------------------------------
void MainWindow::onSurfaceTestNoteOff(uint8_t note)
//-------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  onMidiNoteOffReceived(note, 0x64, &out);
}

//---------------------------------------------------------------------------------
void MainWindow::handleIncomingNote( uint8_t note, uint8_t velocity, IMidiOut* out)
//---------------------------------------------------------------------------------
{
  for (auto* fourTracksTab : fourTracksTabs_)
  {
    const auto curvesKey = curveEditorTab_[0]->curves();
    const auto curvesVel = curveEditorTab_[1]->curves();

    std::vector<uint8_t> v = fourTracksTab->trackIndices();

    for (auto trackIndex : v)
    {
      const GroupMorphProfile gp = GroupMorphProfile::GetProfile(fourTracksTab->getGroupIdByAbsTrack(trackIndex));
      assert(gp.useKey || gp.useVelocity);

      const ExpressionCurveId keyCurveId = fourTracksTab->keyExprCurveId(trackIndex);
      const ExpressionCurveId velCurveId = fourTracksTab->velExprCurveId(trackIndex);

      double r = ExpressionCalculator::computeScaleFactor(keyCurveId, velCurveId, gp, curvesKey, curvesVel, note, velocity);
      uint8_t expr = std::clamp(uint8_t(std::lround(127.0 * r)), (uint8_t)0, (uint8_t)127);

      uint8_t velocity_out = velocity;

      if (playMode_ != PlayMode::Poly )
      {
        sendControlChange(*out, trackIndex, 11, expr);
      }
      else
      {
        // Se non si usa l'expression per miscelare i suoni usiamo la scalatura della velocity
        velocity_out = std::clamp(uint8_t(std::lround(velocity * r)), (uint8_t)1, (uint8_t)127);
        qDebug() << "NOTE ON (velocities evaluation) track: " << trackIndex << ", note: " << note << ", orig.vel: " << velocity << "modified velocity: " << velocity_out;
      }

      double cc71contribute = cc71contribute_[trackIndex] / 64.0;//-64 <--> 63 ==> -1 <--> 1
      double cc74contribute = cc74contribute_[trackIndex] / 64.0;//-64 <--> 63 ==> -1 <--> 1

      uint8_t cc71 = 64 + (expr / 2.0) * cc71contribute;//cc71contribute = 0 --> cc71 = 64; cc71contribute = -1 --> cc71 = 0 - 64; cc71contribute = 1 --> cc71 = 64 - 127;
      uint8_t cc74 = 64 + (expr / 2.0) * cc74contribute;//cc74contribute = 0 --> cc74 = 64; cc74contribute = -1 --> cc74 = 0 - 64; cc74contribute = 1 --> cc74 = 64 - 127;
      cc71 = std::max((uint8_t)0, std::min((uint8_t)127, cc71));
      cc74 = std::max((uint8_t)0, std::min((uint8_t)127, cc74));

      sendControlChange(*out, trackIndex, 71, cc71);
      sendControlChange(*out, trackIndex, 74, cc74);

      // Duplicare la nota sulla traccia.
      sendNoteOn(*out, trackIndex, note + footageTransposition[trackIndex], velocity_out);
    }
  }
}

//-----------------------------------------------------------------------------------
void MainWindow::handleIncomingNoteOff(uint8_t note, uint8_t velocity, IMidiOut* out)
//-----------------------------------------------------------------------------------
{
  for (auto* fourTracksTab : fourTracksTabs_)
  {
    std::vector<uint8_t> v = fourTracksTab->trackIndices();
    for (auto trackIndex : v)
    {
      // Duplicare la nota sulla traccia.
      sendNoteOff(*out, trackIndex, note + footageTransposition[trackIndex], velocity);
    }
  }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::handleIncomingChannelMsg(uint8_t code, uint8_t data1, uint8_t data2, IMidiOut* out)
//--------------------------------------------------------------------------------------------------
{
  for (auto* fourTracksTab : fourTracksTabs_)
  {
    std::vector<uint8_t> v = fourTracksTab->trackIndices();
    for (auto trackIndex : v)
    {
      // Duplicare la nota sulla traccia.
      sendChannelMsg(*out, trackIndex, code, data1, data2);
    }
  }
}

//--------------------------------------------------------------------
PresetData MainWindow::captureCurrentPreset(const QString& name) const
//--------------------------------------------------------------------
{
  return PresetData();
}

//----------------------------------------------------
void MainWindow::applyPreset(const PresetData& preset)
//----------------------------------------------------
{
  // A. MIDI setup globale
  loadingPreset_ = true;
  midiSettingTab_->setFromPreset(preset.appInitSettings);
  loadingPreset_ = false;

  // B. Parametri per traccia
  for (uint8_t trk = 0; trk < 16; trk++)
    fourTracksTabs_[trk/4]->setTrackPresetData(preset.tracks[trk], trk);

  // C. Curve
  curveEditorTab_[0]->setCurves(preset.keyCurves);
  curveEditorTab_[1]->setCurves(preset.velCurves);
  curveEditorTab_[0]->setKeyboardRange(preset.appInitSettings.keyboardRangeId);
  midiSettingTab_->setKeyboardRange(preset.appInitSettings.keyboardRangeId);
}

//---------------------------------------------------
void MainWindow::gatherPresetData(PresetData& preset)
//---------------------------------------------------
{
  // A. MIDI setup globale
  midiSettingTab_->getPresetData(preset.appInitSettings);

  // B. Parametri per traccia
  for (uint8_t trk = 0; trk < 16; trk++)
    fourTracksTabs_[trk / 4]->getTrackPresetData(preset.tracks[trk], trk);

  // C. Curve
  preset.keyCurves = curveEditorTab_[0]->curves();
  preset.velCurves = curveEditorTab_[1]->curves();
  preset.appInitSettings.keyboardRangeId = midiSettingTab_->keyboardRange();
}

//-----------------------------
void MainWindow::onButtonSave()
//-----------------------------
{
  PresetData p;
  gatherPresetData(p);
  p.name = midiSettingTab_->currentPresetName();
  presetManager_->savePreset(p);
}

//-------------------------------
void MainWindow::onButtonSaveAs()
//-------------------------------
{
  QString currentName = midiSettingTab_->currentPresetName();

  bool ok = false;
  QString name = QInputDialog::getText( this,
                                        tr("Save Preset"),
                                        tr("Preset name:"),
                                        QLineEdit::Normal,
                                        currentName.isEmpty() ? tr("New Preset") : currentName,
                                        &ok);

  if (!ok || name.trimmed().isEmpty())
    return;

  name = name.trimmed();

  if (presetManager_->contains(name))
  {
    auto reply = QMessageBox::question(
      this,
      tr("Overwrite preset"),
      tr("Preset \"%1\" already exists. Overwrite it?").arg(name),
      QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
      return;
  }

  PresetData preset;
  gatherPresetData(preset);
  preset.name = name;
  presetManager_->savePreset(preset);

  const auto& presets = presetManager_->presets();
  midiSettingTab_->refreshPresetCombo(presets);
  midiSettingTab_->selectPresetByName(name);
}

//-------------------------------
void MainWindow::onButtonDelete()
//-------------------------------
{
  QString name = midiSettingTab_->currentPresetName();
  if (name.isEmpty())
    return;

  auto reply = QMessageBox::question(
    this,
    tr("Delete preset"),
    tr("Delete preset \"%1\"?").arg(name),
    QMessageBox::Yes | QMessageBox::No
  );

  if (reply != QMessageBox::Yes)
    return;

  presetManager_->removePreset(name);
  presetManager_->saveToDisk();

  refreshPresetUi();
}

//--------------------------------------------
void MainWindow::onPresetSelectionChanged(int)
//--------------------------------------------
{
  midiSettingTab_->updatePresetButtons();

  QString name = midiSettingTab_->currentPresetName();
  if (const PresetData* p = presetManager_->findPreset(name))
  {
    applyPreset(*p);
  }
  else//no preset selection
  {
    for (auto fourTracksTab : fourTracksTabs_)
      for (uint8_t trk = 0; trk < 4; trk++)
        fourTracksTab->resetTrackMidiWidgets(trk);

    memset(footageTransposition, 0, 16);
  }
}

//----------------------------------------
void MainWindow::onPlayModeChanged(int id)
//----------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();

  switch (id)
  {
  case 0: // Mono
    if (playMode_ == PlayMode::Poly)
    {
      for (auto* fourTracksTab : fourTracksTabs_)
      {
        std::vector<uint8_t> v = fourTracksTab->trackIndices();
        for (auto trackIndex : v)
          sendAllNotesOff(out, trackIndex);
      }
    }
    playMode_ = PlayMode::MonoRetrigVelOff;
    break;

  case 1: // Mono retrig (original velocity)
  case 2: // Mono retrig (velcity off)
    if (playMode_ == PlayMode::Poly)
    {
      for (auto* fourTracksTab : fourTracksTabs_)
      {
        std::vector<uint8_t> v = fourTracksTab->trackIndices();
        for (auto trackIndex : v)
          sendAllNotesOff(out, trackIndex);
      }
    }
    playMode_ = (id == 1) ? PlayMode::MonoRetrigOrigVel : PlayMode::MonoRetrigVelOff;
    break;

  case 3: // Poly
    if (playMode_ != PlayMode::Poly)
    {
      for (auto* fourTracksTab : fourTracksTabs_)
      {
        std::vector<uint8_t> v = fourTracksTab->trackIndices();
        for (auto trackIndex : v)
          sendControlChange(out, trackIndex, 11, 0xFF);
      }
    }
    playMode_ = PlayMode::Poly;
    monoPlayingNote_.reset();
    currNotes_.clear();
    break;
  }

  saveInitSettings();
}

//--------------------------------
void MainWindow::refreshPresetUi()
//--------------------------------
{
  midiSettingTab_->refreshPresetUi(presetManager_->presetNames());
}

//-----------------------------------------------------------------
void MainWindow::onTrackFootageChanged(uint8_t trackIdx, Footage f)
//-----------------------------------------------------------------
{
  footageTransposition[trackIdx] = FootageTransposition[(uint8_t)f];

  IMidiOut& out = midiSettingTab_->MidiOut();

  //All notes off (cambiando footageTransposition a cavallo di un note on / note off potrei avare note appese)
  sendAllNotesOff(out, trackIdx);

  //NRPN di fine tuning
  int8_t detune = FootageDetune[(uint8_t)f];
  sendRpnFine(out, trackIdx, detune);
}

//---------------------------------------------------------------------------------------------------
void MainWindow::onTrackProgramChanged(uint8_t trackIdx, uint8_t cc00, uint8_t cc32, uint8_t program)
//---------------------------------------------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  sendControlChange(out, trackIdx, 0, cc00);
  sendControlChange(out, trackIdx, 32, cc32);
  sendProgramChange(out, trackIdx, program);
}

//--------------------------------------------------------------------
void MainWindow::onTrackVolumeChanged(uint8_t trackIdx, uint8_t value)
//--------------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  sendControlChange(out, trackIdx, 7, (uint8_t)value);
}

//-----------------------------------------------------------------
void MainWindow::onTrackPanChanged(uint8_t trackIdx, uint8_t value)
//-----------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  sendControlChange(out, trackIdx, 10, (uint8_t)value);
}

//--------------------------------------------------------------------
void MainWindow::onTrackReverbChanged(uint8_t trackIdx, uint8_t value)
//--------------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  sendControlChange(out, trackIdx, 91, (uint8_t)value);
}

//--------------------------------------------------------------------
void MainWindow::onTrackChorusChanged(uint8_t trackIdx, uint8_t value)
//--------------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  sendControlChange(out, trackIdx, 93, (uint8_t)value);
}

//---------------------------------------------------------------------
void MainWindow::onTrackTimbre1Changed(uint8_t trackIdx, uint8_t value)
//---------------------------------------------------------------------
{
  cc71contribute_[trackIdx] = value;
}

//---------------------------------------------------------------------
void MainWindow::onTrackTimbre2Changed(uint8_t trackIdx, uint8_t value)
//---------------------------------------------------------------------
{
  cc74contribute_[trackIdx] = value;
}

////---------------------------------------
//void MainWindow::loadInstrumentDatabase()
////---------------------------------------
//{
//  QString error;
//  const QString path = ":/Ins/instrument_database.json";
//
//  if (!instrumentDatabase_.loadFromJsonFile(path, &error))
//  {
//    qWarning() << "Failed to load instrument database:" << error;
//    return;
//  }
//
//  if (midiSettingTab_)
//    midiSettingTab_->setKnownInstrumentNames(instrumentDatabase_.instrumentNames());
//}

//-----------------------------------------------
void MainWindow::startInstrumentDatabaseLoading()
//-----------------------------------------------
{
  qDebug() << "BOOT DB background load started";

  if (midiSettingTab_)
    midiSettingTab_->setLoadingDatabase();

  instrumentDbWatcher_ = new QFutureWatcher<std::shared_ptr<InstrumentDatabase>>(this);

  connect(instrumentDbWatcher_, &QFutureWatcher<std::shared_ptr<InstrumentDatabase>>::finished,
    this,
    [this]()
    {
      auto db = instrumentDbWatcher_->result();

      if (db)
      {
        instrumentDatabase_ = db;   // o il tuo membro equivalente
        qDebug() << "BOOT DB background load finished";

        // Qui aggiorni/abiliti le UI che dipendono dagli strumenti.
        if (midiSettingTab_)
          midiSettingTab_->setKnownInstrumentNames(instrumentDatabase_->instrumentNames(), pendingInstrumentName_);
      }
      else
      {
        qDebug() << "BOOT DB background load failed";
      }

      instrumentDbWatcher_->deleteLater();
      instrumentDbWatcher_ = nullptr;
    });

  instrumentDbWatcher_->setFuture(QtConcurrent::run(
    []() -> std::shared_ptr<InstrumentDatabase>
    {
      auto db = std::make_shared<InstrumentDatabase>();
      db->loadFromJsonFile(":/Ins/instrument_database.json"); // qui metti il codice oggi dentro loadInstrumentDatabase()
      return db;
    }));
}

//-------------------------------------------------------------------------------------------------------
void MainWindow::onInstrumentProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program)
//-------------------------------------------------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  sendControlChange(out, trackIdx, 0, msb);
  sendControlChange(out, trackIdx, 32, lsb);
  sendProgramChange(out, trackIdx, program);
}

//---------------------------------------------------------------------------------------------------
void MainWindow::onManualProgramSelected(uint8_t trackIdx, uint8_t msb, uint8_t lsb, uint8_t program)
//---------------------------------------------------------------------------------------------------
{
  IMidiOut& out = midiSettingTab_->MidiOut();
  sendControlChange(out, trackIdx, 0, msb);
  sendControlChange(out, trackIdx, 32, lsb);
  sendProgramChange(out, trackIdx, program);
}

//--------------------------------------------
void MainWindow::onOutPortChanged(uint8_t idx)
//--------------------------------------------
{
  saveInitSettings();
}

//-------------------------------------------
void MainWindow::onInPortChanged(uint8_t idx)
//-------------------------------------------
{
  saveInitSettings();
}

//---------------------------------------------
void MainWindow::onInChannelChanged(uint8_t id)
//---------------------------------------------
{
  saveInitSettings();
}
