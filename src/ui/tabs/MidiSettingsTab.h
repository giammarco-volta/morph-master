#pragma once

#include <QWidget>
#include "../../../../Common/src/midi/IMidiOut.h"
#include "../../../../Common/src/midi/MidiMonoIn.h"
#include "../../core/KeyboardStandardRanges.h"

class MainWindow;
class QComboBox;
class QPushButton;
class QLabel;
class QCheckBox;
class QRadioButton;
class QButtonGroup;
class QString;
class MidiChannelSelector;
struct AppInitSettings;
struct PresetData;

class MidiSettingsTab : public QWidget
{
  Q_OBJECT

public:
  explicit MidiSettingsTab(MainWindow* parent = nullptr);

  IMidiOut& MidiOut() const { return *midiOut_.get(); }

  void setFromPreset(const AppInitSettings& initSettings);
  void getPresetData(AppInitSettings& initSettings) const;

  QString currentPresetName() const;
  void refreshPresetUi(const QStringList& names);
  void updatePresetButtons();
  void refreshPresetCombo(const QVector<PresetData>& presets);
  void selectPresetByName(const QString& name);

  // Instrument definition
  void setLoadingDatabase();
  void setKnownInstrumentNames(const QStringList& names, const QString name);
  bool isInstrumentMode() const;
  QString selectedKnownInstrument() const;

  KeyboardRangeId keyboardRange() const;
  void setKeyboardRange(KeyboardRangeId kr);

private:
  void connectMidiIn(uint8_t deviceIndex);

  void updateInstrumentDefinitionUi();

  void onRefreshPorts();
  void onPortChanged(uint8_t idx);

  void onRefreshInPorts();
  void onInPortChanged(uint8_t idx);
  void onInChannelChanged(uint8_t id);

  void onGenerateJsonButton();
  void onButtonGM2Reset();
  void onButtonSoftReset();
  void onSoftResetChannelChanged(uint8_t id);

  // Instrument definition
  void onInstrumentDefinitionModeChanged();
  void onKnownInstrumentChanged(int idx);

private slots:
  void onKeyboardRangeChanged(uint8_t index);

signals:
  void signalKeyboardRangeChanged(uint8_t index);
  void midiNoteOnReceived(uint8_t note, uint8_t velocity, IMidiOut* out);
  void midiNoteOffReceived(uint8_t note, uint8_t velocity, IMidiOut* out);
  void midiChannelMsgReceived(uint8_t code, uint8_t data1, uint8_t data2, IMidiOut* out);

  // Instrument definition
  void instrumentDefinitionModeChanged(bool instrumentMode);
  void knownInstrumentChanged(const QString& instrumentName);

private:
  std::unique_ptr<IMidiOut>               midiOut_;
  std::unique_ptr<MidiIn_MonoInterpreter> midiIn_;

  QComboBox* outPorts_{};
  QPushButton* outRefresh_{};

  MidiChannelSelector* inChannelSelector_ = nullptr;
  QComboBox* inPorts_{};
  QPushButton* inRefresh_{};

  QComboBox* keyboardRangeCombo_ = nullptr;

  QComboBox* presetCombo_{};
  QPushButton* presetSave_{};
  QPushButton* presetSaveAs_{};
  QPushButton* presetDelete_{};

  // Instrument definition
  QRadioButton* instrumentRadio_{};
  QRadioButton* manualRadio_{};
  QComboBox* knownInstrumentCombo_{};

  QPushButton* gm2ResetButton_{};
  QPushButton* softResetButton_{};
  MidiChannelSelector* softResetChannel_{};

  QRadioButton* polyRadio_{};
  QRadioButton* monoVelOffRetrigRadio_{};
  QRadioButton* monoRadioNoRetrig_{};
  QRadioButton* monoOrigVelRetrigRadio_{};
};