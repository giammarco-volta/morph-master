#include <QApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QDir>
#include <QQuickStyle>
#include <QQmlContext>
#include <QTimer>

#include "controllers/SettingsController.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QtCore/qnativeinterface.h>

static void enableKeepScreenOn()
{
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]()
    {
      QJniObject activity = QNativeInterface::QAndroidApplication::context();

      if (!activity.isValid())
      {
        qWarning() << "Android activity/context not valid";
        return;
      }

      QJniObject window =
        activity.callObjectMethod(
          "getWindow",
          "()Landroid/view/Window;");

      if (!window.isValid())
      {
        qWarning() << "Android window not valid";
        return;
      }

      constexpr int FLAG_KEEP_SCREEN_ON = 128;

      window.callMethod<void>(
        "addFlags",
        "(I)V",
        FLAG_KEEP_SCREEN_ON);

      qDebug() << "FLAG_KEEP_SCREEN_ON set";
    });
}
#endif

int main(int argc, char* argv[])
{
#ifdef Q_OS_ANDROID
  qputenv("QSG_RHI_BACKEND", "opengl");
#endif

  QApplication app(argc, argv);

  QQuickStyle::setStyle("Material");

  QCoreApplication::setOrganizationName("NaadaLab");
  QCoreApplication::setOrganizationDomain("naadalab.com");
  QCoreApplication::setApplicationName("Morphora");

  SettingsController settingsController;

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("SettingsController", &settingsController);

#ifdef NDEBUG
  constexpr bool debugBuild = false;
#else
  constexpr bool debugBuild = true;
#endif
  engine.rootContext()->setContextProperty("DebugBuild", debugBuild);

  engine.loadFromModule("MorphMaster", "Main");

  settingsController.loadInstrumentDatabaseAsync(":/Ins/instrument_database.json");

  if (engine.rootObjects().isEmpty())
    return -1;
  
  settingsController.delayedMidiRefreshAfterStartup();

#ifdef Q_OS_ANDROID
  QTimer::singleShot(500, &app, []()
    {
      enableKeepScreenOn();
    });
#endif

  bool ret = app.exec();
  return ret;
}
