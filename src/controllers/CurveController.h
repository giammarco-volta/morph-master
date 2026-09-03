#pragma once

#include <QObject>
#include <QString>

class SettingsController;

enum class CurveSet
{
  Key,
  Velocity
};

class CurveController : public QObject
{
  Q_OBJECT

    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(bool noteMode READ noteMode CONSTANT)

    Q_PROPERTY(int minX READ minX NOTIFY rangeChanged)
    Q_PROPERTY(int maxX READ maxX NOTIFY rangeChanged)

    Q_PROPERTY(int x1 READ x1 WRITE setX1 NOTIFY x1Changed)
    Q_PROPERTY(int y1 READ y1 WRITE setY1 NOTIFY y1Changed)
    Q_PROPERTY(int x2 READ x2 WRITE setX2 NOTIFY x2Changed)
    Q_PROPERTY(int y2 READ y2 WRITE setY2 NOTIFY y2Changed)

public:
  CurveController(SettingsController& settingsController,
    CurveSet curveSet,
    QObject* parent = nullptr);

  QString title() const;
  bool noteMode() const;

  int minX() const;
  int maxX() const;

  int x1() const;
  void setX1(int value);

  int y1() const;
  void setY1(int value);

  int x2() const;
  void setX2(int value);

  int y2() const;
  void setY2(int value);

  void notifyDataChanged();

signals:
  void rangeChanged();

  void x1Changed();
  void y1Changed();
  void x2Changed();
  void y2Changed();

private:
  bool normalizeXToCurrentRange();

  SettingsController& settingsController_;
  CurveSet curveSet_;
};