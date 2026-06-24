#include "MidiSettingsTab.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSignalBlocker>
#include <QDir>
#include <QFileInfoList>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QScrollArea>
#include <QSizePolicy>

#include <algorithm>
#include <map>
#include <tuple>

#include "../../MainWindow.h"

#include "../../../../Common/src/midi/MidiOutFactory.h"
#include "../../../../Common/src/midi/MidiInFactory.h"
#include "../../../../Common/src/midi/MidiMessage.h"
#include "../../../../Common/src/ui/widgets/MidiChannelSelector.h"
#include "../../../../Common/src/ui/widgets/TouchComboBox.h"

#include "../widgets/MorphSurfaceWidget.h"

#include "../../core/Presets.h"
#include "../../core/InsParser.h"
#include "../../core/JsonSerializer.h"


namespace
{
  using ProgramKey = std::tuple<int, int, int>;

  //------------------------------------------
  ProgramKey programKey(const ProgramEntry& p)
  //------------------------------------------
  {
    return ProgramKey{ p.msb, p.lsb, p.program };
  }

  //-------------------------------------------------------
  void sortProgramsByMidiAddress(InstrumentDefinition& def)
  //-------------------------------------------------------
  {
    std::sort(def.programs.begin(), def.programs.end(),
      [](const ProgramEntry& a, const ProgramEntry& b)
      {
        if (a.msb != b.msb) return a.msb < b.msb;
        if (a.lsb != b.lsb) return a.lsb < b.lsb;
        if (a.program != b.program) return a.program < b.program;
        if (a.bankName != b.bankName) return a.bankName < b.bankName;
        return a.name < b.name;
      });
  }

  //------------------------------------------------------------------------------------------------
  bool hasProgramWithSameMidiAddress(const InstrumentDefinition& def, const ProgramEntry& candidate)
  //------------------------------------------------------------------------------------------------
  {
    const ProgramKey key = programKey(candidate);

    return std::any_of(def.programs.begin(), def.programs.end(),
      [&](const ProgramEntry& existing)
      {
        return programKey(existing) == key;
      });
  }

  //-----------------------
  struct CanonicalModelRule
  //-----------------------
  {
    QString manufacturer;
    QString model;
    QString canonicalName;
    QString normalizedModel;
    QString compactModel;
  };

  //------------------------------
  QString compactSpaces(QString s)
  //------------------------------
  {
    s = s.trimmed();
    s.replace(QRegularExpression("\\s+"), " ");
    return s;
  }

  //----------------------------------
  QString normalizeKNumbers(QString s)
  //----------------------------------
  {
    QRegularExpression re("\\b(\\d+)\\s*k\\b", QRegularExpression::CaseInsensitiveOption);

    auto it = re.globalMatch(s);
    while (it.hasNext())
    {
      const auto m = it.next();
      s.replace(m.captured(0), QString::number(m.captured(1).toInt() * 1000), Qt::CaseInsensitive);
    }

    return s;
  }

  //---------------------------------
  QString normalizedTokens(QString s)
  //---------------------------------
  {
    s = normalizeKNumbers(s);
    s = s.toUpper();
    s.replace(QRegularExpression("[^A-Z0-9]+"), " ");
    return compactSpaces(s);
  }

  //---------------------------
  QString compactKey(QString s)
  //---------------------------
  {
    s = normalizedTokens(s);
    s.remove(' ');
    return s;
  }

  //------------------------------------------------
  QString canonicalManufacturerName(QString section)
  //------------------------------------------------
  {
    section = section.trimmed();

    if (section.compare("CLAVIA / NORD", Qt::CaseInsensitive) == 0)
      return "Nord";

    if (section.compare("EMU", Qt::CaseInsensitive) == 0 ||
      section.compare("E-MU", Qt::CaseInsensitive) == 0)
      return "E-MU";

    return section.left(1).toUpper() + section.mid(1).toLower();
  }

  //-------------------------------------------------------
  std::vector<CanonicalModelRule> loadCanonicalModelRules()
  //-------------------------------------------------------
  {
    QStringList candidatePaths;

    {
      QDir d(QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir());
      candidatePaths << QDir::cleanPath(d.filePath("../../resources/Ins/Models.txt"));
    }

    candidatePaths << QDir::cleanPath(QDir::currentPath() + "/src/resources/Ins/Models.txt");
    candidatePaths << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/src/resources/Ins/Models.txt");
    candidatePaths << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../src/resources/Ins/Models.txt");
    candidatePaths << ":/Ins/Models.txt";
    candidatePaths << ":/resources/Ins/Models.txt";

    QFile file;
    QString usedPath;

    for (const QString& path : candidatePaths)
    {
      file.setFileName(path);

      if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
      {
        usedPath = path;
        break;
      }
    }

    if (!file.isOpen())
    {
      qWarning() << "Models.txt not found. Tried paths:" << candidatePaths;
      return {};
    }

    qDebug() << "Loading canonical instrument models from:" << usedPath;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    std::vector<CanonicalModelRule> rules;
    QString currentManufacturer;

    while (!in.atEnd())
    {
      QString line = in.readLine().trimmed();

      if (line.isEmpty())
        continue;

      if (line.startsWith("--") && line.endsWith("--"))
      {
        line.remove(0, 2);
        line.chop(2);
        currentManufacturer = canonicalManufacturerName(line);
        continue;
      }

      if (currentManufacturer.isEmpty())
        continue;

      CanonicalModelRule rule;
      rule.manufacturer = currentManufacturer;
      rule.model = line;
      rule.canonicalName = currentManufacturer + " " + line;
      rule.normalizedModel = normalizedTokens(line);
      rule.compactModel = compactKey(line);

      rules.push_back(rule);
    }

    qDebug() << "Loaded" << rules.size() << "canonical instrument model rules.";

    return rules;
  }

  //--------------------------------------------------------------------------------------------
  bool modelMatchesInstrumentName(const CanonicalModelRule& rule, const QString& instrumentName)
  //--------------------------------------------------------------------------------------------
  {
    const QString normalizedName = normalizedTokens(instrumentName);
    const QString compactName = compactKey(instrumentName);

    const QString paddedName = " " + normalizedName + " ";
    const QString paddedModel = " " + rule.normalizedModel + " ";

    // Safe token match: useful for short names like M1, T1, WK, etc.
    if (paddedName.contains(paddedModel))
      return true;

    // Compact match: useful for JV-1080 / JV1080, PSR-9000 / PSR9000, etc.
    // Avoid this for very short ambiguous keys.
    if (rule.compactModel.length() >= 4 && compactName.contains(rule.compactModel))
      return true;

    return false;
  }

  //-------------------------------------------------------------------------------------------------------------
  QString canonicalNameForInstrument(const std::string& deviceName, const std::vector<CanonicalModelRule>& rules)
  //-------------------------------------------------------------------------------------------------------------
  {
    const QString originalName = QString::fromStdString(deviceName).trimmed();

    for (const CanonicalModelRule& rule : rules)
    {
      if (modelMatchesInstrumentName(rule, originalName))
        return rule.canonicalName;
    }

    return originalName;
  }

};//namespace

//------------------------------------------------------------------------------------------------------------------------
MidiSettingsTab::MidiSettingsTab(MainWindow* parent) : QWidget(parent), midiOut_(createMidiOut()), midiIn_(createMidiIn())
//------------------------------------------------------------------------------------------------------------------------
{
  auto* outerLayout = new QVBoxLayout(this);
  outerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->setSpacing(0);

  auto* scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

//#ifdef Q_OS_ANDROID
//  QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
//#endif

  auto* content = new QWidget(scrollArea);
  content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  auto* mainLayout = new QVBoxLayout(content);
  mainLayout->setContentsMargins(10, 10, 10, 10);
  mainLayout->setSpacing(10);

  // =========================================================
  // MIDI Settings group
  // =========================================================
  auto* midiSettingsGroup = new QGroupBox(tr("MIDI Setup"), this);
  auto* midiSettingsLayout = new QVBoxLayout(midiSettingsGroup);
  midiSettingsLayout->setSpacing(10);

  {
    auto* box = new QGroupBox(tr("MIDI IN"), midiSettingsGroup);
    auto* layout = new QVBoxLayout(box);

    // Top row: device + refresh
    auto* top = new QHBoxLayout();
    top->addWidget(new QLabel(tr("Device:"), box));

    inPorts_ = new TouchComboBox(box);
    top->addWidget(inPorts_, 1);

    inRefresh_ = new QPushButton(tr("Refresh"), box);
    top->addWidget(inRefresh_);

    layout->addLayout(top);

    // Channel row: touch-friendly 4x4 MIDI channel selector.
    auto* channelRow = new QHBoxLayout();
    channelRow->addWidget(new QLabel(tr("Input channel:"), box));

    inChannelSelector_ = new MidiChannelSelector(box);
    inChannelSelector_->setValue(1);

    channelRow->addWidget(inChannelSelector_);
    channelRow->addStretch();

    layout->addLayout(channelRow);

    midiSettingsLayout->addWidget(box);

    connect(inRefresh_, &QPushButton::clicked, this, &MidiSettingsTab::onRefreshInPorts);
    connect(inPorts_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MidiSettingsTab::onInPortChanged);
    connect(inChannelSelector_, &MidiChannelSelector::valueChanged, this, &MidiSettingsTab::onInChannelChanged);

    connect(inPorts_, qOverload<int>(&QComboBox::currentIndexChanged), parent, &MainWindow::onInPortChanged);
    connect(inChannelSelector_, &MidiChannelSelector::valueChanged, parent, &MainWindow::onInChannelChanged);
  }

  {
    auto* box = new QGroupBox(tr("MIDI OUT"), midiSettingsGroup);
    auto* layout = new QVBoxLayout(box);

    auto* top = new QHBoxLayout();
    top->addWidget(new QLabel(tr("Device:"), box));

    outPorts_ = new TouchComboBox(box);
    top->addWidget(outPorts_, 1);

    outRefresh_ = new QPushButton(tr("Refresh"), box);
    top->addWidget(outRefresh_);

    layout->addLayout(top);

    midiSettingsLayout->addWidget(box);

    connect(outRefresh_, &QPushButton::clicked, this, &MidiSettingsTab::onRefreshPorts);
    connect(outPorts_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MidiSettingsTab::onPortChanged);

    connect(outPorts_, qOverload<int>(&QComboBox::currentIndexChanged), parent, &MainWindow::onOutPortChanged);
  }

  mainLayout->addWidget(midiSettingsGroup);

  // =========================================================
  // Reset group
  // =========================================================
  {
    auto* resetGroup = new QGroupBox(tr("Reset"), this);
    auto* resetLayout = new QHBoxLayout(resetGroup);

#ifndef NDEBUG
    auto* generateJsonButton = new QPushButton(tr("Generate JSON from ins files"), resetGroup);
    resetLayout->addWidget(generateJsonButton);
    connect(generateJsonButton, &QPushButton::pressed, this, &MidiSettingsTab::onGenerateJsonButton);
#endif

    gm2ResetButton_ = new QPushButton(tr("GM2 reset"), resetGroup);
    resetLayout->addWidget(gm2ResetButton_);

    softResetButton_ = new QPushButton(tr("Soft reset (channel 1)"), resetGroup);
    resetLayout->addWidget(softResetButton_);

    softResetChannel_ = new MidiChannelSelector(resetGroup);
    softResetChannel_->setValue(1);
    resetLayout->addWidget(softResetChannel_);

    connect(gm2ResetButton_, &QPushButton::pressed, this, &MidiSettingsTab::onButtonGM2Reset);
    connect(softResetButton_, &QPushButton::pressed, this, &MidiSettingsTab::onButtonSoftReset);
    connect(softResetChannel_, &MidiChannelSelector::valueChanged, this, &MidiSettingsTab::onSoftResetChannelChanged);

    mainLayout->addWidget(resetGroup);
  }

  // =========================================================
  // Instrument definition group
  // =========================================================
  {
    auto* instrumentGroup = new QGroupBox(tr("Patch selecion"), this);
    auto* instrumentLayout = new QGridLayout(instrumentGroup);

    instrumentRadio_ = new QRadioButton(tr("Through instrument definition"), instrumentGroup);
    manualRadio_ = new QRadioButton(tr("Manual (through banks and program number)"), instrumentGroup);

    instrumentRadio_->setChecked(true);

    instrumentLayout->addWidget(instrumentRadio_, 0, 0);
    instrumentLayout->addWidget(manualRadio_, 1, 0);

    instrumentLayout->addWidget(new QLabel(tr("Known instrument"), instrumentGroup), 0, 1);

    knownInstrumentCombo_ = new TouchComboBox(instrumentGroup);
    instrumentLayout->addWidget(knownInstrumentCombo_, 1, 1, 1, 2);
    knownInstrumentCombo_->setCurrentIndex(-1);

    connect(manualRadio_, &QRadioButton::toggled, this, &MidiSettingsTab::onInstrumentDefinitionModeChanged);

    connect(knownInstrumentCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MidiSettingsTab::onKnownInstrumentChanged);

    updateInstrumentDefinitionUi();

    manualRadio_->setChecked(true);

    mainLayout->addWidget(instrumentGroup);
  }

  // =========================================================
  // Keyaboard range group
  // =========================================================
  {
    auto* KeyaboardRangeGroup = new QGroupBox(tr("Keyaboard range"), this);
    auto* layout = new QVBoxLayout(KeyaboardRangeGroup);

    keyboardRangeCombo_ = new TouchComboBox(this);
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::Full].name), int(KeyboardRangeId::Full));
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::K88].name), int(KeyboardRangeId::K88));
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::K76].name), int(KeyboardRangeId::K76));
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::K73].name), int(KeyboardRangeId::K73));
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::K61].name), int(KeyboardRangeId::K61));
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::K49].name), int(KeyboardRangeId::K49));
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::K37].name), int(KeyboardRangeId::K37));
    keyboardRangeCombo_->addItem(tr(kKeyboardRanges[(uint8_t)KeyboardRangeId::K25].name), int(KeyboardRangeId::K25));
    keyboardRangeCombo_->setCurrentIndex(0); // Full

    layout->addWidget(keyboardRangeCombo_);
    mainLayout->addWidget(KeyaboardRangeGroup);
  }

  // =========================================================
  // Play mode group
  // =========================================================
  {
    auto* playModeGroup = new QGroupBox(tr("Play mode"), this);
    auto* playModeLayout = new QVBoxLayout(playModeGroup);

    polyRadio_ = new QRadioButton(tr("Poly"), playModeGroup);
    monoVelOffRetrigRadio_ = new QRadioButton(tr("Mono"), playModeGroup);// with retrig using velocity off

    playModeLayout->addWidget(polyRadio_);
    playModeLayout->addWidget(monoVelOffRetrigRadio_);

    // gruppo logico
    auto* group = new QButtonGroup(this);
    group->addButton(polyRadio_, 0);
    group->addButton(monoVelOffRetrigRadio_, 1);

#ifndef NDEBUG
    monoOrigVelRetrigRadio_ = new QRadioButton(tr("Mono (retrig original velocity)"), playModeGroup);
    monoRadioNoRetrig_ = new QRadioButton(tr("Mono no retrig"), playModeGroup);

    playModeLayout->addWidget(monoOrigVelRetrigRadio_);
    playModeLayout->addWidget(monoRadioNoRetrig_);

    group->addButton(monoOrigVelRetrigRadio_, 2);
    group->addButton(monoRadioNoRetrig_, 3);
#endif

    // segnale unico e pulito
    connect(group, &QButtonGroup::idClicked, parent, &MainWindow::onPlayModeChanged);

    // default
    monoVelOffRetrigRadio_->setChecked(true);

    mainLayout->addWidget(playModeGroup);
  }

  // =========================================================
  // Presets group
  // =========================================================
  {
    auto* presetsGroup = new QGroupBox(tr("Presets"), this);
    auto* presetsLayout = new QHBoxLayout(presetsGroup);

    presetsLayout->addWidget(new QLabel(tr("Preset"), presetsGroup));

    presetCombo_ = new TouchComboBox(presetsGroup);
    presetsLayout->addWidget(presetCombo_, 1);

    presetSave_ = new QPushButton(tr("Save"), presetsGroup);
    presetsLayout->addWidget(presetSave_);

    presetSaveAs_ = new QPushButton(tr("Save As..."), presetsGroup);
    presetsLayout->addWidget(presetSaveAs_);

    presetDelete_ = new QPushButton(tr("Delete"), presetsGroup);
    presetsLayout->addWidget(presetDelete_);

    connect(presetSave_, &QPushButton::pressed, parent, &MainWindow::onButtonSave);
    connect(presetSaveAs_, &QPushButton::pressed, parent, &MainWindow::onButtonSaveAs);
    connect(presetDelete_, &QPushButton::pressed, parent, &MainWindow::onButtonDelete);
    connect(presetCombo_, &QComboBox::currentIndexChanged, parent, &MainWindow::onPresetSelectionChanged);

    mainLayout->addWidget(presetsGroup);
  }

  // =========================================================

  mainLayout->addStretch();
  content->setLayout(mainLayout);
  scrollArea->setWidget(content);
  outerLayout->addWidget(scrollArea);

  onRefreshPorts();
  onRefreshInPorts();

  updatePresetButtons();

  connect(keyboardRangeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MidiSettingsTab::onKeyboardRangeChanged);

  onKeyboardRangeChanged(keyboardRangeCombo_->currentIndex());

#ifdef Q_OS_ANDROID
  // Se c'è una Pa5X nella lista, selezionala automaticamente.
  for (int i = 0; i < inPorts_->count(); ++i)
  {
    if (inPorts_->itemText(i).contains("Pa5X", Qt::CaseInsensitive))
    {
      inPorts_->setCurrentIndex(i);
      break;
    }
  }
  for (int i = 0; i < outPorts_->count(); ++i)
  {
    if (outPorts_->itemText(i).contains("Pa5X", Qt::CaseInsensitive))
    {
      outPorts_->setCurrentIndex(i);
      break;
    }
  }
  qDebug() << "ANDROID selected MIDI IN:" << inPorts_->currentIndex() << inPorts_->currentText();
  qDebug() << "ANDROID selected MIDI OUT:" << outPorts_->currentIndex() << outPorts_->currentText();
#endif
}

//----------------------------------------------------------------------
void MidiSettingsTab::setFromPreset(const AppInitSettings& initSettings)
//----------------------------------------------------------------------
{
  int index = inPorts_->findText(initSettings.midiSetup.midiInPort);
  if (index >= 0)
    inPorts_->setCurrentIndex(index);

  index = outPorts_->findText(initSettings.midiSetup.midiOutPort);
  if (index >= 0)
    outPorts_->setCurrentIndex(index);

  inChannelSelector_->setValue(initSettings.midiSetup.midiInChannel);

  switch (initSettings.playMode)
  {
    case PlayMode::Poly: polyRadio_->setChecked(true); break;
    case PlayMode::MonoRetrigVelOff: monoVelOffRetrigRadio_->setChecked(true); break;
    case PlayMode::MonoRetrigOrigVel: monoOrigVelRetrigRadio_ ? monoOrigVelRetrigRadio_->setChecked(true) : monoVelOffRetrigRadio_->setChecked(true); break;
    case PlayMode::MonoNoRetrig: monoRadioNoRetrig_ ? monoRadioNoRetrig_->setChecked(true) : monoVelOffRetrigRadio_->setChecked(true); break;
  }

  initSettings.patchPolicy == PatchPolicy::Manual ? manualRadio_->setChecked(true) : instrumentRadio_->setChecked(true);

  index = knownInstrumentCombo_->findText(initSettings.knownInstrumentName);
  if (index >= 0)
    knownInstrumentCombo_->setCurrentIndex(index);

  setKeyboardRange(initSettings.keyboardRangeId);
}

//----------------------------------------------------------------------
void MidiSettingsTab::getPresetData(AppInitSettings& initSettings) const
//----------------------------------------------------------------------
{
  initSettings.midiSetup.midiInPort = inPorts_->currentText();
  initSettings.midiSetup.midiOutPort = outPorts_->currentText();
  initSettings.midiSetup.midiInChannel = inChannelSelector_->value();

  if (polyRadio_->isChecked())
    initSettings.playMode = PlayMode::Poly;
  else if (monoVelOffRetrigRadio_->isChecked() || !monoOrigVelRetrigRadio_ || !monoRadioNoRetrig_)
    initSettings.playMode = PlayMode::MonoRetrigVelOff;
  else if (monoOrigVelRetrigRadio_->isChecked())
    initSettings.playMode = PlayMode::MonoRetrigOrigVel;
  else if (monoRadioNoRetrig_->isChecked())
    initSettings.playMode = PlayMode::MonoNoRetrig;

  initSettings.patchPolicy = manualRadio_->isChecked() ? PatchPolicy::Manual : PatchPolicy::Instrument;
  initSettings.knownInstrumentName = knownInstrumentCombo_->currentText();
  initSettings.keyboardRangeId = keyboardRange();
}

//------------------------------------------------
QString MidiSettingsTab::currentPresetName() const
//------------------------------------------------
{
  return presetCombo_->currentText();
}

//-------------------------------------------------------------
void MidiSettingsTab::refreshPresetUi(const QStringList& names)
//-------------------------------------------------------------
{
  QSignalBlocker blocker(presetCombo_);
  presetCombo_->clear();
  presetCombo_->addItems(names);
  presetCombo_->setCurrentIndex(-1);
}

//-----------------------------------------
void MidiSettingsTab::updatePresetButtons()
//-----------------------------------------
{
  const bool hasSelection = presetCombo_ &&
                            presetCombo_->currentIndex() >= 0 &&
                           !presetCombo_->currentText().trimmed().isEmpty();

  presetSave_->setEnabled(hasSelection);
  presetSaveAs_->setEnabled(true);
  presetDelete_->setEnabled(hasSelection);
}

//--------------------------------------------------------------------------
void MidiSettingsTab::refreshPresetCombo(const QVector<PresetData>& presets)
//--------------------------------------------------------------------------
{
  QString current = presetCombo_->currentText();

  QSignalBlocker blocker(presetCombo_);

  presetCombo_->clear();

  for (const auto& p : presets)
    presetCombo_->addItem(p.name);

  // prova a ripristinare selezione
  int index = presetCombo_->findText(current);
  if (index >= 0)
    presetCombo_->setCurrentIndex(index);
  else if (presetCombo_->count() > 0)
    presetCombo_->setCurrentIndex(0);
  else
    presetCombo_->setCurrentIndex(-1);

  updatePresetButtons();
}

//-----------------------------------------------------------
void MidiSettingsTab::selectPresetByName(const QString& name)
//-----------------------------------------------------------
{
  QSignalBlocker blocker(presetCombo_);

  int index = presetCombo_->findText(name);
  if (index >= 0)
    presetCombo_->setCurrentIndex(index);

  updatePresetButtons();
}

//------------------------------------------------------
void MidiSettingsTab::connectMidiIn(uint8_t deviceIndex)
//------------------------------------------------------
{
  if (!midiIn_)
    midiIn_ = createMidiIn();

  if (!midiIn_)
  {
    qDebug() << "MIDI IN: backend not available";
    return;
  }

  midiIn_->close();

  // IMPORTANT: WinMM callback may come from a non-Qt thread.
  // Keep this callback light; if you need to touch UI, forward via invokeMethod queued.
  midiIn_->setCallback([this](const MidiInEvent& ev) {
      auto st = midiIn_->getState();
      if (st.type == EventType::noteOn)
      {
        const uint8_t ch = inChannelSelector_->value() - 1;
        const uint8_t note = st.currentNote;
        const uint8_t vel = st.currentVel;

        QMetaObject::invokeMethod(this, [this, ch, note, vel]() {
          emit midiNoteOnReceived(note, vel, midiOut_.get());
          }, Qt::QueuedConnection);
      }
      else if (st.type == EventType::noteOff)
      {
        const uint8_t ch = inChannelSelector_->value() - 1;
        const uint8_t note = ev.data1;
        const uint8_t vel  = ev.data2;

        QMetaObject::invokeMethod(this, [this, ch, note, vel]() {
          emit midiNoteOffReceived(note, vel, midiOut_.get());
          }, Qt::QueuedConnection);
      }
      else
      {
        const uint8_t ch = inChannelSelector_->value();
        const uint8_t code = ev.message();
        const uint8_t data1 = ev.data1;
        const uint8_t data2 = ev.data2;

        QMetaObject::invokeMethod(this, [this, ch, code, data1, data2]() {
          emit midiChannelMsgReceived(code, data1, data2, midiOut_.get());
          }, Qt::QueuedConnection);

      }
    });

  if (deviceIndex < 0)
  {
    qDebug() << tr("MIDI IN: no device selected");
    return;
  }

  const bool ok = midiIn_->open(deviceIndex);
  qDebug() << (ok ? tr("MIDI IN: connected") : tr("MIDI IN: open failed"));
}

//--------------------------------------
void MidiSettingsTab::onRefreshInPorts()
//--------------------------------------
{
  if (!inPorts_) return;

  if (!midiIn_)
    midiIn_ = createMidiIn();

  inPorts_->clear();

  if (!midiIn_)
  {
    inPorts_->addItem(tr("<MIDI IN backend not available>"));
    qDebug() << tr("MIDI IN: backend not available");
    return;
  }

  const auto ins = midiIn_->listInputs();
  inPorts_->addItems(ins);

  if (ins.size() > 0)
    onInPortChanged(0);
}

//------------------------------------------------
void MidiSettingsTab::onInPortChanged(uint8_t idx)
//-----------------------------------------------
{
  if (!inPorts_)
    return;

  // idx corresponds to midiIn_->listInputs() order
  connectMidiIn(idx);
}

//--------------------------------------------------
void MidiSettingsTab::onInChannelChanged(uint8_t id)
//--------------------------------------------------
{
  // id = 1..16
  if (id < 1 || id > 16)
    return;

  midiIn_->setSourceChannel(id - 1);
}

//--------------------------------------
void MidiSettingsTab::onButtonGM2Reset()
//--------------------------------------
{
  static constexpr uint8_t gm2Reset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };

  std::vector<uint8_t> msg(std::begin(gm2Reset), std::end(gm2Reset));

  midiOut_->sendSysEx(msg);

  presetCombo_->setCurrentIndex(-1);

  updatePresetButtons();
}

//---------------------------------------
void MidiSettingsTab::onButtonSoftReset()
//---------------------------------------
{
  uint8_t ch = softResetChannel_->value() - 1;
  midiOut_->sendShort(0xB0 | (ch & 0x0F), 123, 0);// All Notes Off
  midiOut_->sendShort(0xB0 | (ch & 0x0F), 121, 0);// Reset All Controllers
}

//---------------------------------------------------------
void MidiSettingsTab::onSoftResetChannelChanged(uint8_t id)
//---------------------------------------------------------
{
  softResetButton_->setText(tr("Soft reset (channel %1)").arg(id));
}

//------------------------------------
void MidiSettingsTab::onRefreshPorts()
//------------------------------------
{
  outPorts_->clear();
  auto outs = midiOut_->listOutputs();
  outPorts_->addItems(outs);
}

//----------------------------------------------
void MidiSettingsTab::onPortChanged(uint8_t idx)
//----------------------------------------------
{
  Q_UNUSED(idx);
  if (!midiOut_->open(idx))
    qDebug() << tr("Open failed (or not implemented on this platform)");
}

//----------------------------------------
void MidiSettingsTab::setLoadingDatabase()
//----------------------------------------
{
  knownInstrumentCombo_->setEditText("Loading database...");
}

//-----------------------------------------------------------------------------------------
void MidiSettingsTab::setKnownInstrumentNames(const QStringList& names, const QString name)
//-----------------------------------------------------------------------------------------
{
  if (!knownInstrumentCombo_)
    return;

  QSignalBlocker blocker(knownInstrumentCombo_);
  knownInstrumentCombo_->clear();
  knownInstrumentCombo_->addItems(names);
  int index = knownInstrumentCombo_->findText(name);
  if (index >= 0)
    knownInstrumentCombo_->setCurrentIndex(index);
}

//----------------------------------------------
bool MidiSettingsTab::isInstrumentMode() const
//----------------------------------------------
{
  return instrumentRadio_ && instrumentRadio_->isChecked();
}

//--------------------------------------------------------
QString MidiSettingsTab::selectedKnownInstrument() const
//--------------------------------------------------------
{
  return knownInstrumentCombo_ ? knownInstrumentCombo_->currentText() : QString();
}

//--------------------------------------------------
void MidiSettingsTab::updateInstrumentDefinitionUi()
//--------------------------------------------------
{
  const bool instrumentMode = isInstrumentMode();

  if (knownInstrumentCombo_)
    knownInstrumentCombo_->setEnabled(instrumentMode);
}

//-------------------------------------------------------
void MidiSettingsTab::onInstrumentDefinitionModeChanged()
//-------------------------------------------------------
{
  updateInstrumentDefinitionUi();
  knownInstrumentCombo_->setEnabled(isInstrumentMode());
  if (!isInstrumentMode())
    knownInstrumentCombo_->setCurrentIndex(-1);
  emit instrumentDefinitionModeChanged(isInstrumentMode());
}

//-----------------------------------------------------
void MidiSettingsTab::onKnownInstrumentChanged(int idx)
//-----------------------------------------------------
{
  Q_UNUSED(idx);

  if (!isInstrumentMode())
    return;

  emit knownInstrumentChanged(selectedKnownInstrument());
}

//-----------------------------------------------------------------------------------------------------------------------------------------
void mergeInstrument(std::vector<InstrumentDefinition>& instruments, std::map<QString, size_t>& indexByName, InstrumentDefinition&& source)
//-----------------------------------------------------------------------------------------------------------------------------------------
{
  const QString key = QString::fromStdString(source.deviceName).trimmed().toCaseFolded();

  if (key.isEmpty())
    return;

  const auto it = indexByName.find(key);

  if (it == indexByName.end())
  {
    sortProgramsByMidiAddress(source);
    indexByName[key] = instruments.size();
    instruments.push_back(std::move(source));
    return;
  }

  InstrumentDefinition& target = instruments[it->second];

  for (auto& program : source.programs)
    if (!hasProgramWithSameMidiAddress(target, program))
      target.programs.push_back(std::move(program));

  sortProgramsByMidiAddress(target);
}

//------------------------------------------
void MidiSettingsTab::onGenerateJsonButton()
//------------------------------------------
{
  const QString insDirPath = "E:/Ins";
  const QString outJsonPath = "E:/Ins/instrument_database.json";

  const auto canonicalRules = loadCanonicalModelRules();

  QDir dir(insDirPath);
  if (!dir.exists())
  {
    qWarning() << "INS directory does not exist:" << insDirPath;
    return;
  }

  const QFileInfoList files = dir.entryInfoList(
    QStringList() << "*.ins" << "*.INS",
    QDir::Files | QDir::NoSymLinks,
    QDir::Name);

  if (files.isEmpty())
  {
    qWarning() << "No .ins files found in" << insDirPath;
    return;
  }

  std::vector<InstrumentDefinition> mergedInstruments;
  std::map<QString, size_t> indexByCanonicalName;

  int parsedBeforeMerge = 0;
  int canonicalizedCount = 0;
  int duplicateProgramsSkipped = 0;

  for (const QFileInfo& fi : files)
  {
    qDebug() << "Processing" << fi.absoluteFilePath();

    InsParser parser;
    InsParser::ParseError fatalError;
    auto parsed = parser.parseFile(fi.absoluteFilePath(), fatalError);

    if (!parsed)
    {
      qWarning() << "Fatal parse error in"
        << fi.fileName()
        << ":"
        << QString::fromStdString(fatalError.message);
      continue;
    }

    for (const auto& w : parsed->warnings)
    {
      qWarning() << fi.fileName()
        << (w.line > 0 ? QString("line %1").arg(w.line) : QString())
        << ":"
        << QString::fromStdString(w.message);
    }

    for (auto& instr : parsed->instruments)
    {
      ++parsedBeforeMerge;

      const QString originalName = QString::fromStdString(instr.deviceName).trimmed();
      const QString canonicalName = canonicalNameForInstrument(instr.deviceName, canonicalRules);

      if (!canonicalName.isEmpty() && canonicalName != originalName)
      {
        ++canonicalizedCount;
        instr.deviceName = canonicalName.toStdString();
      }

      const QString key = QString::fromStdString(instr.deviceName).trimmed().toCaseFolded();

      auto it = indexByCanonicalName.find(key);

      if (it == indexByCanonicalName.end())
      {
        sortProgramsByMidiAddress(instr);
        indexByCanonicalName[key] = mergedInstruments.size();
        mergedInstruments.push_back(std::move(instr));
      }
      else
      {
        InstrumentDefinition& target = mergedInstruments[it->second];

        for (auto& program : instr.programs)
        {
          if (hasProgramWithSameMidiAddress(target, program))
          {
            ++duplicateProgramsSkipped;
          }
          else
          {
            target.programs.push_back(std::move(program));
          }
        }

        sortProgramsByMidiAddress(target);
      }
    }
  }

  if (mergedInstruments.empty())
  {
    qWarning() << "No instruments parsed successfully.";
    return;
  }

  std::sort(
    mergedInstruments.begin(),
    mergedInstruments.end(),
    [](const InstrumentDefinition& a, const InstrumentDefinition& b)
    {
      return a.deviceName < b.deviceName;
    });

  const QString namesDumpPath = QFileInfo(outJsonPath).absolutePath() + "/instrument_names_dump.txt";

  QFile namesDumpFile(namesDumpPath);

  if (namesDumpFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream out(&namesDumpFile);
    out.setEncoding(QStringConverter::Utf8);

    for (const InstrumentDefinition& instrument : mergedInstruments)
      out << QString::fromStdString(instrument.deviceName) << "\n";

    qDebug() << "Instrument names dump written to:" << namesDumpPath;
  }
  else
  {
    qWarning() << "Cannot write instrument names dump:" << namesDumpPath;
  }




  const QString programsDumpPath = QFileInfo(outJsonPath).absolutePath() + "/programs_by_category_dump.txt";

  QMap<QString, QStringList> programsByCategory;

  for (const InstrumentDefinition& instrument : mergedInstruments)
  {
    const QString instrumentName = QString::fromStdString(instrument.deviceName);

    for (const ProgramEntry& program : instrument.programs)
    {
      const QString category = QString::fromStdString(category_name[static_cast<uint8_t>(program.category)]);

      const QString programName = QString::fromStdString(program.name);

      programsByCategory[category].append(
        QString("%1 | (%2) MSB=%3 LSB=%4 PRG=%5 | %6")
        .arg(instrumentName)
        .arg(category)
        .arg(program.msb)
        .arg(program.lsb)
        .arg(program.program)
        .arg(programName));
    }
  }

  QFile programsDumpFile(programsDumpPath);

  if (programsDumpFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream out(&programsDumpFile);
    out.setEncoding(QStringConverter::Utf8);

    for (auto it = programsByCategory.cbegin();
      it != programsByCategory.cend();
      ++it)
    {
      out << "\n";
      out << "============================================================\n";
      out << it.key() << "\n";
      out << "============================================================\n\n";

      QStringList entries = it.value();
      entries.sort(Qt::CaseInsensitive);

      for (const QString& entry : entries)
        out << entry << "\n";
    }

    qDebug() << "Programs by category dump written to:" << programsDumpPath;
  }
  else
  {
    qWarning() << "Cannot write programs by category dump:" << programsDumpPath;
  }





  if (!JsonSerializer::saveAllToFile(mergedInstruments, outJsonPath))
  {
    qWarning() << "Failed to write JSON database:" << outJsonPath;
    return;
  }

  qDebug() << "Instrument database written to:" << outJsonPath
    << "with" << mergedInstruments.size() << "instrument definitions"
    << "("
    << parsedBeforeMerge << "parsed before merge,"
    << canonicalizedCount << "canonicalized,"
    << duplicateProgramsSkipped << "duplicate programs skipped"
    << ").";
}


//----------------------------------------------------
KeyboardRangeId MidiSettingsTab::keyboardRange() const
//----------------------------------------------------
{
  if (keyboardRangeCombo_)
    return static_cast<KeyboardRangeId>(keyboardRangeCombo_->currentIndex());
  return KeyboardRangeId();
}

//--------------------------------------------------------
void MidiSettingsTab::setKeyboardRange(KeyboardRangeId kr)
//--------------------------------------------------------
{
  if (keyboardRangeCombo_)
    keyboardRangeCombo_->setCurrentIndex(static_cast<uint8_t>(kr));
}

//---------------------------------------------------------
void MidiSettingsTab::onKeyboardRangeChanged(uint8_t index)
//---------------------------------------------------------
{
  emit signalKeyboardRangeChanged(index);
}
