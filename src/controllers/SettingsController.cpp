#include <QtConcurrent>
#include <QDebug>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <QMetaObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QVariantList>
#include <QVariantMap>
#include <QImageReader>
#include <QSvgRenderer>
#include <QSizeF>

#include <algorithm>
#include <cmath>

#include "SettingsController.h"
#include "TrackController.h"
#include "CurveController.h"
#include "MorphOutputStateModel.h"

#include "../core/ExpressionCalculator.h"
#include "../midi/MidiEngine.h"
#include "../About.h"


namespace
{
  const std::array<QString, 128> kGeneralMidiProgramNames = {
    QStringLiteral("001 Acoustic Grand Piano"),
    QStringLiteral("002 Bright Acoustic Piano"),
    QStringLiteral("003 Electric Grand Piano"),
    QStringLiteral("004 Honky-tonk Piano"),
    QStringLiteral("005 Electric Piano 1"),
    QStringLiteral("006 Electric Piano 2"),
    QStringLiteral("007 Harpsichord"),
    QStringLiteral("008 Clavi"),
    QStringLiteral("009 Celesta"),
    QStringLiteral("010 Glockenspiel"),
    QStringLiteral("011 Music Box"),
    QStringLiteral("012 Vibraphone"),
    QStringLiteral("013 Marimba"),
    QStringLiteral("014 Xylophone"),
    QStringLiteral("015 Tubular Bells"),
    QStringLiteral("016 Dulcimer"),
    QStringLiteral("017 Drawbar Organ"),
    QStringLiteral("018 Percussive Organ"),
    QStringLiteral("019 Rock Organ"),
    QStringLiteral("020 Church Organ"),
    QStringLiteral("021 Reed Organ"),
    QStringLiteral("022 Accordion"),
    QStringLiteral("023 Harmonica"),
    QStringLiteral("024 Tango Accordion"),
    QStringLiteral("025 Acoustic Guitar (nylon)"),
    QStringLiteral("026 Acoustic Guitar (steel)"),
    QStringLiteral("027 Electric Guitar (jazz)"),
    QStringLiteral("028 Electric Guitar (clean)"),
    QStringLiteral("029 Electric Guitar (muted)"),
    QStringLiteral("030 Overdriven Guitar"),
    QStringLiteral("031 Distortion Guitar"),
    QStringLiteral("032 Guitar Harmonics"),
    QStringLiteral("033 Acoustic Bass"),
    QStringLiteral("034 Electric Bass (finger)"),
    QStringLiteral("035 Electric Bass (pick)"),
    QStringLiteral("036 Fretless Bass"),
    QStringLiteral("037 Slap Bass 1"),
    QStringLiteral("038 Slap Bass 2"),
    QStringLiteral("039 Synth Bass 1"),
    QStringLiteral("040 Synth Bass 2"),
    QStringLiteral("041 Violin"),
    QStringLiteral("042 Viola"),
    QStringLiteral("043 Cello"),
    QStringLiteral("044 Contrabass"),
    QStringLiteral("045 Tremolo Strings"),
    QStringLiteral("046 Pizzicato Strings"),
    QStringLiteral("047 Orchestral Harp"),
    QStringLiteral("048 Timpani"),
    QStringLiteral("049 String Ensemble 1"),
    QStringLiteral("050 String Ensemble 2"),
    QStringLiteral("051 Synth Strings 1"),
    QStringLiteral("052 Synth Strings 2"),
    QStringLiteral("053 Choir Aahs"),
    QStringLiteral("054 Voice Oohs"),
    QStringLiteral("055 Synth Voice"),
    QStringLiteral("056 Orchestra Hit"),
    QStringLiteral("057 Trumpet"),
    QStringLiteral("058 Trombone"),
    QStringLiteral("059 Tuba"),
    QStringLiteral("060 Muted Trumpet"),
    QStringLiteral("061 French Horn"),
    QStringLiteral("062 Brass Section"),
    QStringLiteral("063 Synth Brass 1"),
    QStringLiteral("064 Synth Brass 2"),
    QStringLiteral("065 Soprano Sax"),
    QStringLiteral("066 Alto Sax"),
    QStringLiteral("067 Tenor Sax"),
    QStringLiteral("068 Baritone Sax"),
    QStringLiteral("069 Oboe"),
    QStringLiteral("070 English Horn"),
    QStringLiteral("071 Bassoon"),
    QStringLiteral("072 Clarinet"),
    QStringLiteral("073 Piccolo"),
    QStringLiteral("074 Flute"),
    QStringLiteral("075 Recorder"),
    QStringLiteral("076 Pan Flute"),
    QStringLiteral("077 Blown Bottle"),
    QStringLiteral("078 Shakuhachi"),
    QStringLiteral("079 Whistle"),
    QStringLiteral("080 Ocarina"),
    QStringLiteral("081 Lead 1 (square)"),
    QStringLiteral("082 Lead 2 (sawtooth)"),
    QStringLiteral("083 Lead 3 (calliope)"),
    QStringLiteral("084 Lead 4 (chiff)"),
    QStringLiteral("085 Lead 5 (charang)"),
    QStringLiteral("086 Lead 6 (voice)"),
    QStringLiteral("087 Lead 7 (fifths)"),
    QStringLiteral("088 Lead 8 (bass + lead)"),
    QStringLiteral("089 Pad 1 (new age)"),
    QStringLiteral("090 Pad 2 (warm)"),
    QStringLiteral("091 Pad 3 (polysynth)"),
    QStringLiteral("092 Pad 4 (choir)"),
    QStringLiteral("093 Pad 5 (bowed)"),
    QStringLiteral("094 Pad 6 (metallic)"),
    QStringLiteral("095 Pad 7 (halo)"),
    QStringLiteral("096 Pad 8 (sweep)"),
    QStringLiteral("097 FX 1 (rain)"),
    QStringLiteral("098 FX 2 (soundtrack)"),
    QStringLiteral("099 FX 3 (crystal)"),
    QStringLiteral("100 FX 4 (atmosphere)"),
    QStringLiteral("101 FX 5 (brightness)"),
    QStringLiteral("102 FX 6 (goblins)"),
    QStringLiteral("103 FX 7 (echoes)"),
    QStringLiteral("104 FX 8 (sci-fi)"),
    QStringLiteral("105 Sitar"),
    QStringLiteral("106 Banjo"),
    QStringLiteral("107 Shamisen"),
    QStringLiteral("108 Koto"),
    QStringLiteral("109 Kalimba"),
    QStringLiteral("110 Bag Pipe"),
    QStringLiteral("111 Fiddle"),
    QStringLiteral("112 Shanai"),
    QStringLiteral("113 Tinkle Bell"),
    QStringLiteral("114 Agogo"),
    QStringLiteral("115 Steel Drums"),
    QStringLiteral("116 Woodblock"),
    QStringLiteral("117 Taiko Drum"),
    QStringLiteral("118 Melodic Tom"),
    QStringLiteral("119 Synth Drum"),
    QStringLiteral("120 Reverse Cymbal"),
    QStringLiteral("121 Guitar Fret Noise"),
    QStringLiteral("122 Breath Noise"),
    QStringLiteral("123 Seashore"),
    QStringLiteral("124 Bird Tweet"),
    QStringLiteral("125 Telephone Ring"),
    QStringLiteral("126 Helicopter"),
    QStringLiteral("127 Applause"),
    QStringLiteral("128 Gunshot")
  };

  const KeyboardRange& keyboardRangeFor(KeyboardRangeId id)
  {
    for (const auto& r : kKeyboardRanges)
      if (r.id == id)
        return r;

    return kKeyboardRanges[0];
  }

  QString manualImageSourceToQrc(QString src)
  {
    src = src.trimmed();

    if (src.startsWith(QStringLiteral("qrc:/")) ||
      src.startsWith(QStringLiteral(":/")) ||
      src.startsWith(QStringLiteral("http://")) ||
      src.startsWith(QStringLiteral("https://")))
    {
      return src;
    }

    if (src.startsWith(QStringLiteral("./")))
      src.remove(0, 2);

    if (src.startsWith(QStringLiteral("images/")))
      return QStringLiteral("qrc:/manual/") + src;

    return QStringLiteral("qrc:/manual/images/") + src;
  }

  bool isInlineManualImage(const QString& imageTag)
  {
    static const QRegularExpression inlineImageRe(
      QStringLiteral(
        "\\bclass\\s*=\\s*[\"'][^\"']*\\bmanual-inline-image\\b[^\"']*[\"']"),
      QRegularExpression::CaseInsensitiveOption);

    return inlineImageRe.match(imageTag).hasMatch();
  }

  QString decorateManualTextFragment(QString html)
  {
    html.remove(QRegularExpression(
      QStringLiteral("<!DOCTYPE[^>]*>"),
      QRegularExpression::CaseInsensitiveOption));

    html.remove(QRegularExpression(
      QStringLiteral("<head[\\s\\S]*?</head>"),
      QRegularExpression::CaseInsensitiveOption));

    html.remove(QRegularExpression(
      QStringLiteral("</?html[^>]*>"),
      QRegularExpression::CaseInsensitiveOption));

    html.replace(QRegularExpression(
      QStringLiteral("<body[^>]*>"),
      QRegularExpression::CaseInsensitiveOption),
      QString());

    html.replace(QRegularExpression(
      QStringLiteral("</body>"),
      QRegularExpression::CaseInsensitiveOption),
      QString());

    return QStringLiteral(R"(
                              <style>
                                body, p, div, span, li, td {
                                  color: #E8E8E8;
                                  background-color: transparent;
                                }

                                h1, h2, h3 {
                                  color: #D8B85A;
                                }

                                b, strong {
                                  color: #FFFFFF;
                                }

                                i {
                                  color: #B8B8B8;
                                }

                                a {
                                  color: #D8B85A;
                                  text-decoration: none;
                                  font-weight: bold;
                                }
                              </style>
                              <div style="color:#E8E8E8;">
                              )")
                              + html +
                              QStringLiteral("</div>");
  }

  bool extractLeadingCaption(const QString& html, QString& captionHtml, qsizetype& consumedChars)
  {
    static const QRegularExpression captionRe(
      QStringLiteral(
        "^\\s*<p\\b[^>]*class\\s*=\\s*[\"'][^\"']*\\bcaption\\b[^\"']*[\"'][^>]*>"
        "([\\s\\S]*?)"
        "</p>"),
      QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = captionRe.match(html);

    if (!match.hasMatch())
      return false;

    captionHtml = match.captured(1).trimmed();
    consumedChars = match.capturedEnd();

    return true;
  }

  QString extractFirstImageSource(const QString& html)
  {
    static const QRegularExpression imgRe(
      QStringLiteral(R"(<img\b[^>]*\bsrc\s*=\s*["']([^"']+)["'][^>]*>)"),
      QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = imgRe.match(html);

    if (!match.hasMatch())
      return QString();

    return match.captured(1).trimmed();
  }

  QString extractFigcaptionHtml(const QString& html)
  {
    static const QRegularExpression captionRe(
      QStringLiteral(R"(<figcaption\b[^>]*>([\s\S]*?)</figcaption>)"),
      QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = captionRe.match(html);

    if (!match.hasMatch())
      return QString();

    return match.captured(1).trimmed();
  }

  QString qrcUrlToResourcePath(QString source)
  {
    source = source.trimmed();

    if (source.startsWith(QStringLiteral("qrc:/")))
      return QStringLiteral(":") + source.mid(4); // qrc:/manual/... -> :/manual/...

    return source;
  }

  QSizeF manualImageNaturalSize(const QString& qrcSource)
  {
    const QString resourcePath = qrcUrlToResourcePath(qrcSource);

    if (resourcePath.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive))
    {
      QSvgRenderer renderer(resourcePath);

      if (renderer.isValid())
      {
        const QSize defaultSize = renderer.defaultSize();

        if (!defaultSize.isEmpty())
          return QSizeF(defaultSize);

        const QRectF viewBox = renderer.viewBoxF();

        if (!viewBox.isEmpty())
          return viewBox.size();
      }
    }

    QImageReader reader(resourcePath);
    const QSize size = reader.size();

    if (!size.isEmpty())
      return QSizeF(size);

    return QSizeF(600.0, 400.0);
  }
}

//-----------------------------------------------------------------------
SettingsController::SettingsController(QObject* parent) : QObject(parent)
//-----------------------------------------------------------------------
{
  morphOutputGains_.reserve(8);
  for (int i = 0; i < 8; ++i)
    morphOutputGains_.append(0.0);

  loadAppInitSettings();

  connect(&presetManager_, &PresetManager::presetsChanged, this, [this]()
                                                                 {
                                                                   emit presetNamesChanged();

                                                                   const QString currentName = currentPreset_.name.trimmed();

                                                                   int newIndex = -1;

                                                                   if (!currentName.isEmpty())
                                                                   {
                                                                     const QStringList names = presetManager_.presetNames();
                                                                     newIndex = names.indexOf(currentName);
                                                                   }

                                                                   if (currentPresetIndex_ != newIndex)
                                                                   {
                                                                     currentPresetIndex_ = newIndex;
                                                                     emit currentPresetIndexChanged();
                                                                   }
                                                                 });

  presetManager_.loadFromDisk();

  for (int i = 0; i < 16; ++i)
    tracks_[i] = new TrackController(*this, i, this);

  morphOutputStateModel_ = new MorphOutputStateModel(this);
  morphOutputStateModel_->setAssignments(currentPreset_.tracks);
  morphOutputStateModel_->setSpecificNames(currentPreset_.morphOutputs);
  morphOutputStateModel_->setMuteSoloMasks(currentPreset_.appInitSettings.morphOutputMuteMask,
                                           currentPreset_.appInitSettings.morphOutputSoloMask);

  keyCurveController = new CurveController(*this, CurveSet::Key, this);

  velocityCurveController = new CurveController(*this, CurveSet::Velocity, this);

  startMidiEngine();
  syncMidiEngineSnapshot();
}

//---------------------------------------
SettingsController::~SettingsController()
//---------------------------------------
{
  stopMidiEngine();
}

//----------------------------------------
void SettingsController::startMidiEngine()
//----------------------------------------
{
  if (midiThread_)
    return;

  midiThread_ = new QThread(this);
  midiThread_->setObjectName(QStringLiteral("MidiEngineThread"));

  midiEngine_ = new MidiEngine;
  midiEngine_->moveToThread(midiThread_);

  connect(midiThread_, &QThread::started, midiEngine_, &MidiEngine::start);
  connect(midiThread_, &QThread::finished, midiEngine_, &QObject::deleteLater);

  connect(midiEngine_, &MidiEngine::midiInputPortsChanged,
          this, [this](const QStringList& ports)
                {
                  if (midiInputPorts_ == ports)
                    return;

                  midiInputPorts_ = ports;
                  emit midiInputPortsChanged();
                });

  connect(midiEngine_, &MidiEngine::midiOutputPortsChanged,
          this, [this](const QStringList& ports)
                {
                  if (midiOutputPorts_ == ports)
                    return;

                  midiOutputPorts_ = ports;
                  emit midiOutputPortsChanged();
                });

  connect(midiEngine_, &MidiEngine::monitorFeedbackSnapshot,
          this,
          [this](const QVariantList& notes,
                 const QVariantList& gains,
                 quint64 sequence)
          {
            monitorFeedbackNotes_ = notes;
            morphOutputGains_ = gains;

            // Real-time gain is consumed only by Morph Monitor. Updating the
            // shared model here would also wake hidden Curve Editor canvases.
            emit monitorFeedbackChanged(notes, gains);

            if (midiEngine_)
            {
              QMetaObject::invokeMethod(
                midiEngine_,
                [engine = midiEngine_, sequence]()
                {
                  engine->acknowledgeMonitorFeedback(sequence);
                },
                Qt::QueuedConnection);
            }
          });

  connect(midiEngine_, &MidiEngine::midiInputReceivedWithNoAssignedTracks, this, &SettingsController::midiInputReceivedWithNoAssignedTracks);

  connect(midiEngine_, &MidiEngine::midiInPortResolved,
    this, [this](const QString& portName)
    {
      auto& midi = currentPreset_.appInitSettings.midiSetup;

      if (midi.midiInPort == portName)
        return;

      midi.midiInPort = portName;
      saveAppInitSettings();

      emit midiInPortChanged();
    });

  connect(midiEngine_, &MidiEngine::midiOutPortResolved,
    this, [this](const QString& portName)
    {
      auto& midi = currentPreset_.appInitSettings.midiSetup;

      if (midi.midiOutPort == portName)
        return;

      midi.midiOutPort = portName;
      saveAppInitSettings();

      emit midiOutPortChanged();
    });

  midiThread_->start(QThread::TimeCriticalPriority);
}

//---------------------------------------
void SettingsController::stopMidiEngine()
//---------------------------------------
{
  if (!midiThread_)
    return;

  if (midiEngine_)
    QMetaObject::invokeMethod(midiEngine_, &MidiEngine::stop, Qt::BlockingQueuedConnection);

  midiThread_->quit();
  midiThread_->wait();

  midiThread_ = nullptr;
  midiEngine_ = nullptr;
}

//-------------------------------------------------------------------
MidiEngineSnapshot SettingsController::makeMidiEngineSnapshot() const
//-------------------------------------------------------------------
{
  MidiEngineSnapshot s;

  const auto& app = currentPreset_.appInitSettings;

  s.midiInPort = app.midiSetup.midiInPort;
  s.midiOutPort = app.midiSetup.midiOutPort;
  s.midiInChannel = app.midiSetup.midiInChannel;
  s.filterPresetControlChanges =
    app.midiSetup.filterPresetControlChanges;

  s.playMode = app.playMode;
  s.pitchBendRange = app.pitchBendRange;
  s.morphOutputMuteMask = app.morphOutputMuteMask;
  s.morphOutputSoloMask = app.morphOutputSoloMask;

  s.surfaceMinNote = surfaceMinNote();
  s.surfaceMaxNote = surfaceMaxNote();

  s.tracks = currentPreset_.tracks;
  s.keyCurve = currentPreset_.keyCurve;
  s.velCurve = currentPreset_.velCurve;

  return s;
}

//-----------------------------------------------
void SettingsController::syncMidiEngineSnapshot()
//-----------------------------------------------
{
  if (!midiEngine_)
    return;

  const MidiEngineSnapshot snapshot = makeMidiEngineSnapshot();

  QMetaObject::invokeMethod(midiEngine_, [engine = midiEngine_, snapshot]()
                                         {
                                           engine->setSnapshot(snapshot);
                                         },
                                         Qt::QueuedConnection);
}

//--------------------------------------------
QString SettingsController::midiInPort() const
//--------------------------------------------
{
  return currentPreset_.appInitSettings.midiSetup.midiInPort;
}

//-------------------------------------------------------------
void SettingsController::setMidiInPort(const QString& portName)
//-------------------------------------------------------------
{
  auto& midi = currentPreset_.appInitSettings.midiSetup;

  if (midi.midiInPort == portName)
    return;

  midi.midiInPort = portName;

  saveAppInitSettings();
  syncMidiEngineSnapshot();

  emit midiInPortChanged();
}

//---------------------------------------------
QString SettingsController::midiOutPort() const
//---------------------------------------------
{
  return currentPreset_.appInitSettings.midiSetup.midiOutPort;
}

//--------------------------------------------------------------
void SettingsController::setMidiOutPort(const QString& portName)
//--------------------------------------------------------------
{
  auto& midi = currentPreset_.appInitSettings.midiSetup;

  if (midi.midiOutPort == portName)
    return;

  midi.midiOutPort = portName;

  saveAppInitSettings();
  syncMidiEngineSnapshot();

  emit midiOutPortChanged();
}

//--------------------------------------------------------
bool SettingsController::filterPresetControlChanges() const
//--------------------------------------------------------
{
  return currentPreset_.appInitSettings.midiSetup
    .filterPresetControlChanges;
}

//---------------------------------------------------------------------
void SettingsController::setFilterPresetControlChanges(bool enabled)
//---------------------------------------------------------------------
{
  auto& value = currentPreset_.appInitSettings.midiSetup
    .filterPresetControlChanges;

  if (value == enabled)
    return;

  value = enabled;

  saveAppInitSettings();
  syncMidiEngineSnapshot();

  emit filterPresetControlChangesChanged();
}

//-------------------------------------------
int SettingsController::midiInChannel() const
//-------------------------------------------
{
  return currentPreset_.appInitSettings.midiSetup.midiInChannel;
}

//----------------------------------------------------
void SettingsController::setMidiInChannel(int channel)
//----------------------------------------------------
{
  auto& midi = currentPreset_.appInitSettings.midiSetup;

  const int newChannel = std::clamp(channel, 0, 15);

  if (midi.midiInChannel == newChannel)
    return;

  midi.midiInChannel = static_cast<uint8_t>(newChannel);

  saveAppInitSettings();
  syncMidiEngineSnapshot();

  emit midiInChannelChanged();
}

//----------------------------------------------------
QStringList SettingsController::midiInputPorts() const
//----------------------------------------------------
{
  return midiInputPorts_;
}

//----------------------------------------------------
QStringList SettingsController::midiOutputPorts() const
//----------------------------------------------------
{
  return midiOutputPorts_;
}

//-------------------------------------------------------
void SettingsController::delayedMidiRefreshAfterStartup()
//-------------------------------------------------------
{
#ifdef Q_OS_ANDROID
  QTimer::singleShot(300, this, [this]()
    {
      refreshMidiPorts();
    });

  QTimer::singleShot(1000, this, [this]()
    {
      refreshMidiPorts();
    });

  QTimer::singleShot(2500, this, [this]()
    {
      refreshMidiPorts();
    });
#else
  refreshMidiPorts();
#endif
}

//----------------------------------------------------------------------------------------
bool SettingsController::sendTrackControlChange(int trackIndex, uint8_t cc, uint8_t value)
//----------------------------------------------------------------------------------------
{
  if (!midiEngine_)
    return false;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, trackIndex, cc, value]()
    {
      engine->sendTrackControlChange(trackIndex, cc, value);
    },
    Qt::QueuedConnection);

  return true;
}

//------------------------------------------------------------------------------
bool SettingsController::sendTrackProgramChange(int trackIndex, uint8_t program)
//------------------------------------------------------------------------------
{
  if (!midiEngine_)
    return false;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, trackIndex, program]()
    {
      engine->sendTrackProgramChange(trackIndex, program);
    },
    Qt::QueuedConnection);

  return true;
}

//-----------------------------------------------------------------------------------------------------------------------
bool SettingsController::sendTrackBankSelectAndProgram(int trackIndex, uint8_t bankMSB, uint8_t bankLSB, uint8_t program)
//-----------------------------------------------------------------------------------------------------------------------
{
  if (!midiEngine_)
    return false;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, trackIndex, bankMSB, bankLSB, program]()
    {
      engine->sendTrackBankSelectAndProgram(trackIndex, bankMSB, bankLSB, program);
    },
    Qt::QueuedConnection);

  return true;
}

//---------------------------------------------
PresetData& SettingsController::currentPreset()
//---------------------------------------------
{
  return currentPreset_;
}

//---------------------------------------------------------
const PresetData& SettingsController::currentPreset() const
//---------------------------------------------------------
{
  return currentPreset_;
}

//-----------------------------------------
int SettingsController::patchPolicy() const
//-----------------------------------------
{
  return static_cast<int>(currentPreset_.appInitSettings.patchPolicy);
}

//------------------------------------------------
void SettingsController::setPatchPolicy(int value)
//------------------------------------------------
{
  const auto newPolicy = static_cast<PatchPolicy>(value);

  if (currentPreset_.appInitSettings.patchPolicy == newPolicy)
    return;

  const bool oldUseInstrumentDefinition = useInstrumentDefinition();

  currentPreset_.appInitSettings.patchPolicy = newPolicy;

  saveAppInitSettings();

  emit patchPolicyChanged();

  if (oldUseInstrumentDefinition != useInstrumentDefinition())
    emit useInstrumentDefinitionChanged();
}

//------------------------------------------------------
bool SettingsController::useInstrumentDefinition() const
//------------------------------------------------------
{
  return currentPreset_.appInitSettings.patchPolicy == PatchPolicy::Instrument;
}

//---------------------------------------------------------------
void SettingsController::setUseInstrumentDefinition(bool enabled)
//---------------------------------------------------------------
{
  const PatchPolicy newPolicy = enabled ? PatchPolicy::Instrument : PatchPolicy::Manual;

  setPatchPolicy(static_cast<int>(newPolicy));
}

//---------------------------------------------
int SettingsController::keyboardRangeId() const
//---------------------------------------------
{
  return static_cast<int>(currentPreset_.appInitSettings.keyboardRangeId);
}

//----------------------------------------------------
void SettingsController::setKeyboardRangeId(int value)
//----------------------------------------------------
{
  const auto newRange = static_cast<KeyboardRangeId>(value);

  if (currentPreset_.appInitSettings.keyboardRangeId == newRange)
    return;

  currentPreset_.appInitSettings.keyboardRangeId = newRange;

  saveAppInitSettings();
  syncMidiEngineSnapshot();

  emit keyboardRangeIdChanged();
  emit surfaceKeyboardRangeChanged();
}


//---------------------------------------------
int SettingsController::pitchBendRange() const
//---------------------------------------------
{
  return static_cast<int>(currentPreset_.appInitSettings.pitchBendRange);
}

//----------------------------------------------------
void SettingsController::setPitchBendRange(int value)
//----------------------------------------------------
{
  const uint8_t newRange = static_cast<uint8_t>(std::clamp(value, 0, 24));

  if (currentPreset_.appInitSettings.pitchBendRange == newRange)
    return;

  currentPreset_.appInitSettings.pitchBendRange = newRange;

  saveAppInitSettings();
  syncMidiEngineSnapshot();

  emit pitchBendRangeChanged();
}

//--------------------------------------
int SettingsController::playMode() const
//--------------------------------------
{
  return static_cast<int>(currentPreset_.appInitSettings.playMode);
}

//---------------------------------------------
void SettingsController::setPlayMode(int value)
//---------------------------------------------
{
  const auto newPlayMode = static_cast<PlayMode>(value);

  if (currentPreset_.appInitSettings.playMode == newPlayMode)
    return;

  currentPreset_.appInitSettings.playMode = newPlayMode;

  saveAppInitSettings();
  syncMidiEngineSnapshot();

  emit playModeChanged();
}

//-------------------------------------------------------
QObject* SettingsController::track(int trackNumber) const
//-------------------------------------------------------
{
  if (trackNumber < 1 || trackNumber > 16)
    return nullptr;

  return tracks_[trackNumber - 1];
}

//---------------------------------------------------------------------------------------------
bool SettingsController::loadInstrumentDatabase(const QString& filePath, QString* errorMessage)
//---------------------------------------------------------------------------------------------
{
  if (!instrumentDatabase_.loadFromJsonFile(filePath, errorMessage))
    return false;

  emit instrumentNamesChanged();

  const QStringList names = instrumentDatabase_.instrumentNames();
  const QString currentName = currentPreset_.appInitSettings.knownInstrumentName;

  if (!names.isEmpty() &&
    (currentName.trimmed().isEmpty() ||
      instrumentDatabase_.findByName(currentName) == nullptr))
  {
    setKnownInstrumentName(names.first());
  }
  else
  {
    emit knownInstrumentNameChanged();
  }

  return true;
}

//-----------------------------------------------------
QStringList SettingsController::instrumentNames() const
//-----------------------------------------------------
{
  return instrumentDatabase_.instrumentNames();
}

//--------------------------------------------------------------------------
QStringList SettingsController::findInstrumentNames(const QString& nameFilter) const
//--------------------------------------------------------------------------
{
  const QStringList names = instrumentDatabase_.instrumentNames();
  const QStringList tokens = nameFilter.simplified().split(' ', Qt::SkipEmptyParts);

  if (tokens.isEmpty())
    return names;

  QStringList result;
  result.reserve(names.size());

  for (const QString& name : names)
  {
    bool matchesAllTokens = true;
    for (const QString& token : tokens)
    {
      if (!name.contains(token, Qt::CaseInsensitive))
      {
        matchesAllTokens = false;
        break;
      }
    }

    if (matchesAllTokens)
      result.push_back(name);
  }

  return result;
}

//-----------------------------------------------------
QString SettingsController::knownInstrumentName() const
//-----------------------------------------------------
{
  return currentPreset_.appInitSettings.knownInstrumentName;
}

//------------------------------------------------------------------
void SettingsController::setKnownInstrumentName(const QString& name)
//------------------------------------------------------------------
{
  const QString cleanName = name.trimmed();

  if (currentPreset_.appInitSettings.knownInstrumentName == cleanName)
    return;

  currentPreset_.appInitSettings.knownInstrumentName = cleanName;

  saveAppInitSettings();

  emit knownInstrumentNameChanged();
}

//---------------------------------------------------------------------------------
const InstrumentDefinition* SettingsController::currentInstrumentDefinition() const
//---------------------------------------------------------------------------------
{
  return instrumentDatabase_.findByName(currentPreset_.appInitSettings.knownInstrumentName);
}

//--------------------------------------------------------
bool SettingsController::instrumentDatabaseLoading() const
//--------------------------------------------------------
{
  return instrumentDatabaseLoading_;
}

//---------------------------------------------------------
QString SettingsController::instrumentDatabaseError() const
//---------------------------------------------------------
{
  return instrumentDatabaseError_;
}

//---------------------------------------------------------------------------
void SettingsController::loadInstrumentDatabaseAsync(const QString& filePath)
//---------------------------------------------------------------------------
{
  if (instrumentDatabaseLoading_)
    return;

  instrumentDatabaseLoading_ = true;
  instrumentDatabaseError_.clear();

  emit instrumentDatabaseLoadingChanged();
  emit instrumentDatabaseErrorChanged();

  auto* watcher = new QFutureWatcher<InstrumentDatabaseLoadResult>(this);

  connect(watcher,
    &QFutureWatcher<InstrumentDatabaseLoadResult>::finished,
    this,
    [this, watcher]()
    {
      InstrumentDatabaseLoadResult result = watcher->result();
      watcher->deleteLater();

      if (!result.ok)
      {
        instrumentDatabaseLoading_ = false;
        instrumentDatabaseError_ = result.errorMessage;

        qWarning() << "Cannot load instrument database:"
          << instrumentDatabaseError_;

        emit instrumentDatabaseLoadingChanged();
        emit instrumentDatabaseErrorChanged();
        return;
      }

      instrumentDatabase_ = std::move(result.database);
      instrumentDatabaseError_.clear();

      emit instrumentDatabaseErrorChanged();
      emit instrumentNamesChanged();

      const QStringList names = instrumentDatabase_.instrumentNames();
      const QString currentName =
        currentPreset_.appInitSettings.knownInstrumentName.trimmed();

      if (!names.isEmpty() &&
        (currentName.isEmpty() ||
          instrumentDatabase_.findByName(currentName) == nullptr))
      {
        setKnownInstrumentName(names.first());
      }
      else
      {
        emit knownInstrumentNameChanged();
      }

      instrumentDatabaseLoading_ = false;
      emit instrumentDatabaseLoadingChanged();
    });

  watcher->setFuture(QtConcurrent::run([filePath]()
    {
      InstrumentDatabaseLoadResult result;

      QString error;
      InstrumentDatabase database;

      result.ok = database.loadFromJsonFile(filePath, &error);
      result.errorMessage = error;

      if (result.ok)
        result.database = std::move(database);

      return result;
    }));
}

//-------------------------------------------
void SettingsController::refreshMidiInPorts()
//-------------------------------------------
{
  refreshMidiPorts();
}

//--------------------------------------------
void SettingsController::refreshMidiOutPorts()
//--------------------------------------------
{
  refreshMidiPorts();
}

//-----------------------------------------
void SettingsController::refreshMidiPorts()
//-----------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(midiEngine_, &MidiEngine::refreshMidiPorts, Qt::QueuedConnection);
}

//-------------------------------------------
QObject* SettingsController::keyCurve() const
//-------------------------------------------
{
  return keyCurveController;
}

//------------------------------------------------
QObject* SettingsController::velocityCurve() const
//------------------------------------------------
{
  return velocityCurveController;
}

//--------------------------------------------
void SettingsController::loadAppInitSettings()
//--------------------------------------------
{
  QSettings settings;
  settings.beginGroup("AppInitSettings");
  auto& app = currentPreset_.appInitSettings;
  app.midiSetup.midiInPort = settings.value("midiInPort", app.midiSetup.midiInPort).toString();
  app.midiSetup.midiOutPort = settings.value("midiOutPort", app.midiSetup.midiOutPort).toString();
  app.midiSetup.midiInChannel =  static_cast<uint8_t>(std::clamp(settings.value("midiInChannel", int(app.midiSetup.midiInChannel)).toInt(), 0, 15));
  app.midiSetup.filterPresetControlChanges =
    settings.value("filterPresetControlChanges",
                   app.midiSetup.filterPresetControlChanges).toBool();
  app.playMode = static_cast<PlayMode>(settings.value("playMode", int(app.playMode)).toInt());
  app.patchPolicy = static_cast<PatchPolicy>(settings.value("patchPolicy", int(app.patchPolicy)).toInt());
  app.knownInstrumentName = settings.value("knownInstrumentName", app.knownInstrumentName).toString();
  app.keyboardRangeId = static_cast<KeyboardRangeId>(settings.value("keyboardRangeId", int(app.keyboardRangeId)).toInt());
  app.pitchBendRange = static_cast<uint8_t>(std::clamp(settings.value("pitchBendRange", int(app.pitchBendRange)).toInt(), 0, 24));
  app.morphOutputMuteMask = static_cast<uint8_t>(std::clamp(settings.value("morphOutputMuteMask", int(app.morphOutputMuteMask)).toInt(), 0, 255));
  app.morphOutputSoloMask = static_cast<uint8_t>(std::clamp(settings.value("morphOutputSoloMask", int(app.morphOutputSoloMask)).toInt(), 0, 255));
  settings.endGroup();
}

//--------------------------------------------------
void SettingsController::saveAppInitSettings() const
//--------------------------------------------------
{
  QSettings settings;
  settings.beginGroup("AppInitSettings");
  const auto& app = currentPreset_.appInitSettings;
  settings.setValue("midiInPort", app.midiSetup.midiInPort);
  settings.setValue("midiOutPort", app.midiSetup.midiOutPort);
  settings.setValue("midiInChannel", int(app.midiSetup.midiInChannel));
  settings.setValue("filterPresetControlChanges",
                    app.midiSetup.filterPresetControlChanges);
  settings.setValue("playMode", int(app.playMode));
  settings.setValue("patchPolicy", int(app.patchPolicy));
  settings.setValue("knownInstrumentName", app.knownInstrumentName);
  settings.setValue("keyboardRangeId",  int(app.keyboardRangeId));
  settings.setValue("pitchBendRange", int(app.pitchBendRange));
  settings.setValue("morphOutputMuteMask", int(app.morphOutputMuteMask));
  settings.setValue("morphOutputSoloMask", int(app.morphOutputSoloMask));
  settings.endGroup();
}

//--------------------------------------------
int SettingsController::surfaceMinNote() const
//--------------------------------------------
{
  const auto& r = keyboardRangeFor(currentPreset_.appInitSettings.keyboardRangeId);
  return r.minNote;
}

//--------------------------------------------
int SettingsController::surfaceMaxNote() const
//--------------------------------------------
{
  const auto& r = keyboardRangeFor(currentPreset_.appInitSettings.keyboardRangeId);
  return r.maxNote;
}

//------------------------------------------------------------------
QStringList SettingsController::surfaceMorphOutputTrackTexts() const
//------------------------------------------------------------------
{
  QStringList result;

  for (int g = 0; g < static_cast<int>(MorphOutputId::None); ++g)
  {
    QStringList tracks;

    for (int t = 0; t < 16; ++t)
    {
      if (currentPreset_.tracks[t].morphOutput == static_cast<MorphOutputId>(g))
        tracks << QString::number(t + 1);
    }

    result << tracks.join(QStringLiteral(", "));
  }

  return result;
}

//-------------------------------------------------------------------------
QString SettingsController::automaticTrackProgramName(int trackIndex) const
//-------------------------------------------------------------------------
{
  if (trackIndex < 0 || trackIndex >= static_cast<int>(currentPreset_.tracks.size()))
    return {};

  const auto& track = currentPreset_.tracks[trackIndex];

  if (useInstrumentDefinition())
  {
    if (const InstrumentDefinition* def = currentInstrumentDefinition())
    {
      for (const ProgramEntry& program : def->programs)
      {
        if (program.msb == track.program_cc0
            && program.lsb == track.program_cc32
            && program.program == track.program_number)
        {
          const QString name = QString::fromStdString(program.name).trimmed();
          if (!name.isEmpty())
            return name;
        }
      }
    }
  }

  const int programIndex = std::clamp(static_cast<int>(track.program_number), 0, 127);
  return kGeneralMidiProgramNames[programIndex];
}

//---------------------------------------------------------------------------------
void SettingsController::initializeMorphOutputNameIfNeeded(int morphOutputIndex,
                                                            int trackIndex)
//---------------------------------------------------------------------------------
{
  if (morphOutputIndex < 0 || morphOutputIndex >= static_cast<int>(currentPreset_.morphOutputs.size()))
    return;

  auto& output = currentPreset_.morphOutputs[morphOutputIndex];
  if (!output.name.trimmed().isEmpty())
    return;

  output.name = automaticTrackProgramName(trackIndex).trimmed().left(40);
  output.customName = false;

  if (morphOutputStateModel_)
    morphOutputStateModel_->setSpecificNames(currentPreset_.morphOutputs);
}

//-------------------------------------------------------------------------
void SettingsController::setMorphOutputName(int morphOutputIndex,
                                             const QString& name)
//-------------------------------------------------------------------------
{
  if (morphOutputIndex < 0 || morphOutputIndex >= static_cast<int>(currentPreset_.morphOutputs.size()))
    return;

  auto& output = currentPreset_.morphOutputs[morphOutputIndex];
  QString cleanName = name.trimmed().left(40);

  if (cleanName.isEmpty())
  {
    for (int trackIndex = 0; trackIndex < static_cast<int>(currentPreset_.tracks.size()); ++trackIndex)
    {
      if (currentPreset_.tracks[trackIndex].morphOutput == static_cast<MorphOutputId>(morphOutputIndex))
      {
        cleanName = automaticTrackProgramName(trackIndex).trimmed().left(40);
        break;
      }
    }
    output.customName = false;
  }
  else
  {
    output.customName = true;
  }

  if (output.name == cleanName)
    return;

  output.name = cleanName;
  if (morphOutputStateModel_)
    morphOutputStateModel_->setSpecificNames(currentPreset_.morphOutputs);
}

//-----------------------------------------------------------------------
void SettingsController::notifySurfaceMorphOutputsChanged(int trackIndex)
//-----------------------------------------------------------------------
{
  if (midiEngine_)
    midiEngine_->sendAllNotesOffAndExpressionReset(trackIndex);

  if (morphOutputStateModel_)
    morphOutputStateModel_->setAssignments(currentPreset_.tracks);

  if (morphOutputStateModel_)
    morphOutputStateModel_->setSpecificNames(currentPreset_.morphOutputs);

  emit surfaceMorphOutputsChanged();
}

//---------------------------------------------------------------
void SettingsController::surfacePress(double xNorm, double yNorm)
//---------------------------------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, xNorm, yNorm]()
    {
      engine->surfacePress(xNorm, yNorm);
    },
    Qt::QueuedConnection);
}

//--------------------------------------------------------------
void SettingsController::surfaceMove(double xNorm, double yNorm)
//--------------------------------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, xNorm, yNorm]()
    {
      engine->surfaceMove(xNorm, yNorm);
    },
    Qt::QueuedConnection);
}

//---------------------------------------
void SettingsController::surfaceRelease()
//---------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(midiEngine_, &MidiEngine::surfaceRelease, Qt::QueuedConnection);
}

//---------------------------------------------------------
void SettingsController::testNoteOn(int note, int velocity)
//---------------------------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, note, velocity]()
    {
      engine->testNoteOn(note, velocity);
    },
    Qt::QueuedConnection);
}

//-----------------------------------------------
void SettingsController::testNoteOff(int note)
//-----------------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, note]()
    {
      engine->testNoteOff(note);
    },
    Qt::QueuedConnection);
}

//-----------------------------------------------------------------
QVariantList SettingsController::currentMorphOutputGains() const
//-----------------------------------------------------------------
{
  return morphOutputGains_;
}

//-------------------------------------------------------------------
QVariantList SettingsController::currentMonitorFeedbackNotes() const
//-------------------------------------------------------------------
{
  return monitorFeedbackNotes_;
}

//-------------------------------------------------------------------
QVariantList SettingsController::surfaceMorphOutputTrackMasks() const
//-------------------------------------------------------------------
{
  QVariantList result;

  const int morphOutputCount = static_cast<int>(MorphOutputId::None);

  for (int morphOutput = 0; morphOutput < morphOutputCount; ++morphOutput)
  {
    int mask = 0;

    for (int t = 0; t < 16; ++t)
      if (currentPreset_.tracks[t].morphOutput == static_cast<MorphOutputId>(morphOutput))
        mask |= (1 << t);

    result << mask;
  }

  return result;
}

//---------------------------------------------------------------
QAbstractItemModel* SettingsController::morphOutputStateModel() const
//---------------------------------------------------------------
{
  return morphOutputStateModel_;
}

int SettingsController::morphOutputMuteMask() const
{
  return currentPreset_.appInitSettings.morphOutputMuteMask;
}

int SettingsController::morphOutputSoloMask() const
{
  return currentPreset_.appInitSettings.morphOutputSoloMask;
}

void SettingsController::setMorphOutputMuted(int morphOutputIndex, bool muted)
{
  if (morphOutputIndex < 0 || morphOutputIndex >= static_cast<int>(MorphOutputId::None))
    return;

  auto& mask = currentPreset_.appInitSettings.morphOutputMuteMask;
  const uint8_t bit = static_cast<uint8_t>(1u << morphOutputIndex);
  const uint8_t newMask = muted ? static_cast<uint8_t>(mask | bit)
                                : static_cast<uint8_t>(mask & ~bit);
  if (newMask == mask)
    return;

  mask = newMask;
  saveAppInitSettings();
  if (morphOutputStateModel_)
    morphOutputStateModel_->setMuteSoloMasks(mask, currentPreset_.appInitSettings.morphOutputSoloMask);
  emit morphOutputMuteSoloChanged();
  syncMidiEngineSnapshot();
}

void SettingsController::setMorphOutputSolo(int morphOutputIndex, bool solo)
{
  if (morphOutputIndex < 0 || morphOutputIndex >= static_cast<int>(MorphOutputId::None))
    return;

  auto& mask = currentPreset_.appInitSettings.morphOutputSoloMask;
  const uint8_t bit = static_cast<uint8_t>(1u << morphOutputIndex);
  const uint8_t newMask = solo ? static_cast<uint8_t>(mask | bit)
                               : static_cast<uint8_t>(mask & ~bit);
  if (newMask == mask)
    return;

  mask = newMask;
  saveAppInitSettings();
  if (morphOutputStateModel_)
    morphOutputStateModel_->setMuteSoloMasks(currentPreset_.appInitSettings.morphOutputMuteMask, mask);
  emit morphOutputMuteSoloChanged();
  syncMidiEngineSnapshot();
}

//-------------------------------------------------------
void SettingsController::notifyMidiRelevantStateChanged()
//-------------------------------------------------------
{
  syncMidiEngineSnapshot();
}

//-------------------------------------------------------------------------
void SettingsController::setSurfaceMorphOutputTrackMask(int morphOutputIndex, int mask)
//-------------------------------------------------------------------------
{
  const int morphOutputCount = static_cast<int>(MorphOutputId::None);

  if (morphOutputIndex < 0 || morphOutputIndex >= morphOutputCount)
    return;

  const auto targetMorphOutput = static_cast<MorphOutputId>(morphOutputIndex);

  for (int t = 0; t < 16; ++t)
  {
    const bool shouldBelongToMorphOutput = (mask & (1 << t)) != 0;

    if (shouldBelongToMorphOutput)
    {
      if (tracks_[t])
        tracks_[t]->setMorphOutput(morphOutputIndex);
    }
    else
    {
      if (currentPreset_.tracks[t].morphOutput == targetMorphOutput)
      {
        if (tracks_[t])
          tracks_[t]->setMorphOutput(static_cast<int>(MorphOutputId::None));
      }
    }
  }

  emit surfaceMorphOutputsChanged();
}

//-------------------------------------
void SettingsController::sendGM2Reset()
//-------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(midiEngine_, &MidiEngine::sendGM2Reset, Qt::QueuedConnection);
}

//-------------------------------------------------
void SettingsController::sendSoftReset(int channel)
//-------------------------------------------------
{
  if (!midiEngine_)
    return;

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, channel]()
    {
      engine->sendSoftReset(channel);
    },
    Qt::QueuedConnection);
}

//-------------------------------------------------
QStringList SettingsController::presetNames() const
//-------------------------------------------------
{
  return presetManager_.presetNames();
}

//------------------------------------------------------------------
QStringList SettingsController::findPresetNames(const QString& nameFilter) const
//------------------------------------------------------------------
{
  const QStringList names = presetManager_.presetNames();
  const QStringList tokens = nameFilter.simplified().split(' ', Qt::SkipEmptyParts);

  if (tokens.isEmpty())
    return names;

  QStringList result;
  result.reserve(names.size());

  for (const QString& name : names)
  {
    bool matchesAllTokens = true;
    for (const QString& token : tokens)
    {
      if (!name.contains(token, Qt::CaseInsensitive))
      {
        matchesAllTokens = false;
        break;
      }
    }

    if (matchesAllTokens)
      result.push_back(name);
  }

  return result;
}

//------------------------------------------------
int SettingsController::currentPresetIndex() const
//------------------------------------------------
{
  return currentPresetIndex_;
}

//---------------------------------------------------
QString SettingsController::currentPresetName() const
//---------------------------------------------------
{
  return currentPreset_.name;
}

//----------------------------------------------------
QString SettingsController::currentPresetNotes() const
//----------------------------------------------------
{
  return currentPreset_.notes;
}

//------------------------------------------------------------------
void SettingsController::setCurrentPresetNotes(const QString& notes)
//------------------------------------------------------------------
{
  if (currentPreset_.notes == notes)
    return;

  currentPreset_.notes = notes;
  emit currentPresetNotesChanged();
}

//------------------------------------------------------------------
bool SettingsController::presetNameExists(const QString& name) const
//------------------------------------------------------------------
{
  return presetManager_.contains(name.trimmed());
}

//---------------------------------------------------
void SettingsController::activatePreset(int index)
//---------------------------------------------------
{
  const QStringList names = presetManager_.presetNames();

  if (index < 0 || index >= names.size())
    return;

  /*
   * L'utente ha riselezionato il preset corrente:
   * non ricarichiamo il file e non alteriamo lo stato
   * dell'app; reinviamo soltanto il setup MIDI corrente.
   */
  if (currentPresetIndex_ == index)
  {
    sendCurrentPresetSetupToMidiEngine();
    return;
  }

  const PresetData* preset =
    presetManager_.findPreset(names[index]);

  if (!preset)
    return;

  currentPresetIndex_ = index;

  applyPreset(*preset);

  emit currentPresetIndexChanged();
}

//-------------------------------------------------------
void SettingsController::setCurrentPresetIndex(int index)
//-------------------------------------------------------
{
  const QStringList names = presetManager_.presetNames();

  if (index < 0 || index >= names.size())
  {
    if (currentPresetIndex_ == -1)
      return;

    currentPresetIndex_ = -1;
    currentPreset_.name.clear();
    currentPreset_.notes.clear();

    emit currentPresetIndexChanged();
    emit currentPresetNameChanged();
    emit currentPresetNotesChanged();

    return;
  }

  /*
   * Le assegnazioni programmatiche dello stesso valore,
   * usate per sincronizzare la GUI, non devono provocare
   * un reinvio MIDI.
   */
  if (currentPresetIndex_ == index)
    return;

  activatePreset(index);
}

//-----------------------------------------------------------
void SettingsController::sendCurrentPresetSetupToMidiEngine()
//-----------------------------------------------------------
{
  if (!midiEngine_)
    return;

  const MidiEngineSnapshot snapshot = makeMidiEngineSnapshot();

  QMetaObject::invokeMethod(
    midiEngine_,
    [engine = midiEngine_, snapshot]()
    {
      engine->sendAllNotesOffAndExpressionReset();
      engine->setSnapshot(snapshot);
      engine->sendCurrentPresetTrackSetup();
    },
    Qt::QueuedConnection);
}

//------------------------------------------------------------
void SettingsController::applyPreset(const PresetData& preset)
//------------------------------------------------------------
{
  currentPreset_ = preset;

  saveAppInitSettings();

  emitCurrentPresetStateChanged();

  sendCurrentPresetSetupToMidiEngine();
}

//------------------------------------------
bool SettingsController::saveCurrentPreset()
//------------------------------------------
{
  const QString cleanName = currentPreset_.name.trimmed();

  if (cleanName.isEmpty())
    return false;

  currentPreset_.name = cleanName;

  if (!presetManager_.savePreset(currentPreset_))
    return false;

  const QStringList names = presetManager_.presetNames();
  const int newIndex = names.indexOf(cleanName);

  if (currentPresetIndex_ != newIndex)
  {
    currentPresetIndex_ = newIndex;
    emit currentPresetIndexChanged();
  }

  emit currentPresetNameChanged();

  return true;
}

//---------------------------------------------------------------
bool SettingsController::saveCurrentPresetAs(const QString& name)
//---------------------------------------------------------------
{
  const QString cleanName = name.trimmed();

  if (cleanName.isEmpty())
    return false;

  currentPreset_.name = cleanName;

  if (!presetManager_.savePreset(currentPreset_))
    return false;

  const QStringList names = presetManager_.presetNames();
  const int newIndex = names.indexOf(cleanName);

  if (currentPresetIndex_ != newIndex)
  {
    currentPresetIndex_ = newIndex;
    emit currentPresetIndexChanged();
  }

  emit currentPresetNameChanged();

  return true;
}

//--------------------------------------------
bool SettingsController::deleteCurrentPreset()
//--------------------------------------------
{
  const QString name = currentPreset_.name.trimmed();

  if (name.isEmpty())
    return false;

  if (!presetManager_.removePreset(name))
    return false;

  currentPreset_.name.clear();
  currentPreset_.notes.clear();

  currentPresetIndex_ = -1;

  emit currentPresetIndexChanged();
  emit currentPresetNameChanged();
  emit currentPresetNotesChanged();

  return true;
}

//------------------------------------------------------
void SettingsController::emitCurrentPresetStateChanged()
//------------------------------------------------------
{
  emit midiInPortChanged();
  emit midiOutPortChanged();
  emit filterPresetControlChangesChanged();
  emit midiInChannelChanged();

  emit patchPolicyChanged();
  emit useInstrumentDefinitionChanged();
  emit keyboardRangeIdChanged();
  emit pitchBendRangeChanged();
  emit playModeChanged();

  emit knownInstrumentNameChanged();

  emit surfaceKeyboardRangeChanged();
  emit surfaceMorphOutputsChanged();
  emit morphOutputMuteSoloChanged();

  if (morphOutputStateModel_)
  {
    morphOutputStateModel_->setAssignments(currentPreset_.tracks);
    morphOutputStateModel_->setSpecificNames(currentPreset_.morphOutputs);
    morphOutputStateModel_->setMuteSoloMasks(currentPreset_.appInitSettings.morphOutputMuteMask,
                                             currentPreset_.appInitSettings.morphOutputSoloMask);
    morphOutputStateModel_->resetGains();
  }

  emit currentPresetNameChanged();
  emit currentPresetNotesChanged();

  for (auto* t : tracks_)
  {
    if (t)
      t->notifyDataChanged();
  }

  if (keyCurveController)
    keyCurveController->notifyDataChanged();

  if (velocityCurveController)
    velocityCurveController->notifyDataChanged();
}

//-------------------------------------------
QString SettingsController::aboutHtml() const
//-------------------------------------------
{
  return QString::fromUtf8(aboutHTML);
}

//-------------------------------------------------------
QVariantList SettingsController::userManualBlocks() const
//-------------------------------------------------------
{
  QVariantList blocks;

  QFile file(QStringLiteral(":/manual/MorphoraUserManual.html"));

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QVariantMap block;
    block["type"] = QStringLiteral("text");
    block["html"] =
      QStringLiteral("<h1>User Manual</h1>"
        "<p>Unable to load the user manual.</p>");

    blocks << block;
    return blocks;
  }

  const QString html = QString::fromUtf8(file.readAll());

  static const QRegularExpression blockRe(
    QStringLiteral(
      R"((<figure\b[^>]*>[\s\S]*?</figure>)|(<img\b[^>]*\bsrc\s*=\s*["']([^"']+)["'][^>]*>))"),
    QRegularExpression::CaseInsensitiveOption);

  qsizetype pos = 0;
  auto it = blockRe.globalMatch(html);

  while (it.hasNext())
  {
    const QRegularExpressionMatch m = it.next();

    // Le piccole immagini inline devono rimanere nel testo RichText.
    if (isInlineManualImage(m.captured(0)))
      continue;

    const qsizetype start = m.capturedStart();
    const qsizetype end = m.capturedEnd();

    const QString textPart = html.mid(pos, start - pos).trimmed();

    if (!textPart.isEmpty())
    {
      QVariantMap textBlock;
      textBlock["type"] = QStringLiteral("text");
      textBlock["html"] = decorateManualTextFragment(textPart);
      blocks << textBlock;
    }

    const QString figureHtml = m.captured(1);

    QString imageSource;
    QString captionHtml;

    if (!figureHtml.isEmpty())
    {
      imageSource = extractFirstImageSource(figureHtml);
      captionHtml = extractFigcaptionHtml(figureHtml);
    }
    else
    {
      imageSource = m.captured(3).trimmed();
    }

    if (!imageSource.isEmpty())
    {
      QVariantMap imageBlock;
      const QString qrcSource = manualImageSourceToQrc(imageSource);
      const QSizeF naturalSize = manualImageNaturalSize(qrcSource);

      imageBlock["type"] = QStringLiteral("image");
      imageBlock["source"] = qrcSource;
      imageBlock["naturalWidth"] = naturalSize.width();
      imageBlock["naturalHeight"] = naturalSize.height();

      if (!captionHtml.isEmpty())
      {
        imageBlock["captionHtml"] =
          QStringLiteral(
            "<div style=\"color:#B8B8B8; font-style:italic; font-size:13px;\">")
          + captionHtml
          + QStringLiteral("</div>");
      }

      blocks << imageBlock;
    }

    pos = end;
  }

  const QString finalText = html.mid(pos).trimmed();

  if (!finalText.isEmpty())
  {
    QVariantMap textBlock;
    textBlock["type"] = QStringLiteral("text");
    textBlock["html"] = decorateManualTextFragment(finalText);
    blocks << textBlock;
  }

  return blocks;
}