#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

class SettingsController;

//------------------------------------
class TrackController : public QObject
//------------------------------------
{
  Q_OBJECT

  Q_PROPERTY(int morphOutput
    READ morphOutput
    WRITE setMorphOutput
    NOTIFY morphOutputChanged)

  Q_PROPERTY(int footage
    READ footage
    WRITE setFootage
    NOTIFY footageChanged)

  Q_PROPERTY(int detuneOffset
    READ detuneOffset
    WRITE setDetuneOffset
    NOTIFY detuneOffsetChanged)

  Q_PROPERTY(int detuneSpread
    READ detuneSpread
    WRITE setDetuneSpread
    NOTIFY detuneSpreadChanged)

  Q_PROPERTY(int volume
    READ volume
    WRITE setVolume
    NOTIFY volumeChanged)

  Q_PROPERTY(int pan
    READ pan
    WRITE setPan
    NOTIFY panChanged)

  Q_PROPERTY(int reverb
    READ reverb
    WRITE setReverb
    NOTIFY reverbChanged)

  Q_PROPERTY(int chorus
    READ chorus
    WRITE setChorus
    NOTIFY chorusChanged)

  Q_PROPERTY(int tone
    READ tone
    WRITE setTone
    NOTIFY toneChanged)

  Q_PROPERTY(int timbre
    READ timbre
    WRITE setTimbre
    NOTIFY timbreChanged)

  Q_PROPERTY(int instrumentProgramCount
    READ instrumentProgramCount
    NOTIFY instrumentProgramsChanged)

  Q_PROPERTY(QString instrumentProgramDisplayName
    READ instrumentProgramDisplayName
    NOTIFY instrumentProgramDisplayNameChanged)

  Q_PROPERTY(int bankMSB
    READ bankMSB
    WRITE setBankMSB
    NOTIFY bankMSBChanged)

  Q_PROPERTY(int bankLSB
    READ bankLSB
    WRITE setBankLSB
    NOTIFY bankLSBChanged)

  Q_PROPERTY(int programNumber
    READ programNumber
    WRITE setProgramNumber
    NOTIFY programNumberChanged)

public:
  TrackController(SettingsController& settingsController,
    int trackIndex,
    QObject* parent = nullptr);

  int morphOutput() const;
  void setMorphOutput(int value);

  int footage() const;
  void setFootage(int value);

  int detuneOffset() const;
  void setDetuneOffset(int value);

  int detuneSpread() const;
  void setDetuneSpread(int value);

  int volume() const;
  void setVolume(int value);

  int pan() const;
  void setPan(int value);

  int reverb() const;
  void setReverb(int value);

  int chorus() const;
  void setChorus(int value);

  int tone() const;
  void setTone(int value);

  int timbre() const;
  void setTimbre(int value);

  // Case Instrument definition
  int instrumentProgramCount() const;
  QString instrumentProgramDisplayName() const;
  Q_INVOKABLE QVariantList findInstrumentPrograms(const QString& nameFilter, int programNumber) const;
  Q_INVOKABLE void selectInstrumentProgram(int bankMSB, int bankLSB, int programNumber);

  //Case manual
  int bankMSB() const;
  void setBankMSB(int value);

  int bankLSB() const;
  void setBankLSB(int value);

  int programNumber() const;
  void setProgramNumber(int value);

  void notifyDataChanged();

signals:
  void morphOutputChanged();

  void footageChanged();
  void detuneOffsetChanged();
  void detuneSpreadChanged();

  void instrumentProgramsChanged();
  void instrumentProgramDisplayNameChanged();

  void bankMSBChanged();
  void bankLSBChanged();
  void programNumberChanged();

  void volumeChanged();
  void panChanged();
  void reverbChanged();
  void chorusChanged();
  void toneChanged();
  void timbreChanged();

private:
  SettingsController& settingsController_;
  int trackIndex_ = 0; // 0..15
  int8_t cc71contribute_ = 0;
  int8_t cc74contribute_ = 0;
};