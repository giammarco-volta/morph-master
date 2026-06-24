#include <QApplication>
#include "MainWindow.h"
#include "StyleUtils.h"
#include <QFile>
#include <QDir>

#ifdef Q_OS_ANDROID
#include <QFont>
#endif

// IMPORTANTISSIMO:
#include <QtCore/qresource.h>   // oppure #include <QResource>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setStyle("Fusion");

    QString styleSheet = loadStyleSheet(":/qdarkstyle/darkstyle.qss");

#ifdef Q_OS_ANDROID
    styleSheet += R"(

  QWidget {
      font-size: 18pt;
  }

  QPushButton,
  QLineEdit,
  QSpinBox,
  QDoubleSpinBox,
  QTabBar::tab {
      min-height: 44px;
      padding: 6px 10px;
  }

  QComboBox {
      min-height: 44px;
      padding: 4px 10px;
  }

  QScrollBar:vertical {
      width: 32px;
  }

  QScrollBar:horizontal {
      height: 32px;
  }
  #MidiValueSelectorPopup,
  #MidiChannelSelectorPopup {
      background-color: #2b2b2b;
      border: 1px solid #555555;
      border-radius: 6px;
  }

)";
#endif

  //QComboBox QAbstractItemView {
  //    font-size: 18pt;
  //    min-height: 44px;
  //    selection-background-color: #D8B85A;
  //}

    app.setStyleSheet(styleSheet);
    MainWindow w;
    w.show();

    return app.exec();
}