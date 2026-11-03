#include "../src/controllers/SettingsController.h"
#include "../src/controllers/CurveController.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlAbstractUrlInterceptor>
#include <QFontDatabase>
#include <QSettings>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QDir>
#include <iostream>
#include <stdexcept>
#include <cmath>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
class SourceQmlInterceptor : public QQmlAbstractUrlInterceptor {
public:
  QUrl intercept(const QUrl& url, DataType) override {
    const QString prefix="qrc:/qt/qml/src/qml/";
    if(url.toString().startsWith(prefix))
      return QUrl::fromLocalFile(QStringLiteral(MORPH_SOURCE_DIR)+"/src/qml/"+url.toString().mid(prefix.size()));
    return url;
  }
};
static QQuickItem* findVisualItem(QQuickItem* root,const QString& name) {
  if(root->objectName()==name) return root;
  for(auto* child:root->childItems()) if(auto* found=findVisualItem(child,name)) return found;
  return nullptr;
}
static void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
static void settle() { QEventLoop loop; QTimer::singleShot(150,&loop,&QEventLoop::quit);loop.exec(); }
int main(int argc,char** argv) {
  #ifdef _MSC_VER
  _set_error_mode(_OUT_TO_STDERR);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
  #endif
  qputenv("QT_LOGGING_TO_CONSOLE","1");
  qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& message) {
    fprintf(stderr,"%s\n",message.toUtf8().constData()); fflush(stderr);
  });
  fprintf(stderr,"UI test starting\n"); fflush(stderr);
  QGuiApplication app(argc,argv);
  QQuickStyle::setStyle("Material");
#ifdef Q_OS_WIN
  QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR")+"/Fonts/arial.ttf");
#endif
  QTemporaryDir settingsDir;
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settingsDir.path());
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("MorphoraTests");
  QCoreApplication::setApplicationName("CurveUiTests");
  qmlRegisterModule("MorphMaster",1,0);
  qmlRegisterModuleImport("MorphMaster",QQmlModuleImportModuleAny,"NaadaLab.Ui",1,0);
  SettingsController settings;
  SourceQmlInterceptor interceptor;
  QQmlEngine engine;
  engine.addUrlInterceptor(&interceptor);
  engine.rootContext()->setContextProperty("SettingsController",&settings);
  QQmlComponent component(&engine);
  const QString source=QStringLiteral(R"QML(
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import NaadaLab.Ui as SharedUi
import "%1/src/qml/pages" as Pages
ApplicationWindow {
    width: 1280; height: 720; visible: true; font.family: "Arial"
    Material.theme: Material.Dark
    property int layoutClass: SharedUi.UiMetrics.Tablet
    property var panel: null
    function openModalPanel(c,p) {
        panel=c.createObject(contentItem,p)
        panel.x=(width-panel.width)/2; panel.y=(height-panel.height)/2
    }
    function closeModalPanel() { if(panel) panel.destroy(); panel=null }
    Pages.CurvesSettingsPage { anchors.fill: parent; objectName: "curvesPage" }

}
)QML").arg(QUrl::fromLocalFile(QStringLiteral(MORPH_SOURCE_DIR)).toString());
  component.setData(source.toUtf8(),QUrl::fromLocalFile(QStringLiteral(MORPH_SOURCE_DIR)+"/tests/UiHarness.qml"));
  std::unique_ptr<QObject> root(component.create());
  if(!root) { std::cerr<<component.errorString().toStdString();return 1; }
  try {
    settle();
    auto* window=qobject_cast<QQuickWindow*>(root.get());
    auto* page=root->findChild<QQuickItem*>("curvesPage");
    check(page!=nullptr,"Curve page missing");
    for(int slot=0;slot<4;++slot) {
      auto* choice=findVisualItem(window->contentItem(),"curveCcAssignment"+QString::number(slot));
      auto* value=findVisualItem(window->contentItem(),"curveCcValue"+QString::number(slot));
      check(choice && value,"Inline CC widgets missing");
      auto p=value->mapToItem(page,QPointF(0,0));
      check(p.y()>=0 && p.y()+value->height()<=page->height(),"CC value clipped");
    }
    settings.setCurveControl(0,20,0,127);settle();
    auto* controller=qobject_cast<CurveController*>(settings.keyCurve());
    controller->setPoints(40,100,80,0);settle();
    auto* value=findVisualItem(window->contentItem(),"curveCcValue0");
    value->setProperty("value",65000);settle();
    check(std::abs(controller->x1()-45)<0.001 && std::abs(controller->x2()-85)<0.001,"Inline position field did not edit stored curve");
    settings.previewCurveControlValue(0,1);settle();
    check(std::abs(controller->x1()-45)<0.001,"First MIDI message changed edited curve");
    settings.previewCurveControlValue(0,2);settle();
    check(std::abs(controller->x1()-46)<0.001,"MIDI delta did not update points");
    check(value->property("value").toInt()==66000,"MIDI delta did not update position field");
    for(int slot=0;slot<4;++slot) {
      auto* channel=findVisualItem(window->contentItem(),"curveCcChannel"+QString::number(slot));
      check(channel,"Channel combo missing");
      settings.setCurveControlChannel(slot,slot+2);
      settings.setCurveControl(slot,7,0,127);
    }
    settle();
    for(int slot=0;slot<4;++slot) {
      const auto mapping=settings.curveControls()[slot].toMap();
      check(mapping["cc"].toInt()==7 && mapping["channel"].toInt()==slot+2,"Independent CC7 assignment failed");
      auto* channel=findVisualItem(window->contentItem(),"curveCcChannel"+QString::number(slot));
      check(channel->property("currentIndex").toInt()==slot+3,"Channel combo not synchronized");
    }
    // A rapid stream must preserve every bounded delta without replaying every
    // intermediate value through QML. Include reversals after reaching a limit.
    controller->setPoints(40,100,80,0);settle();
    settings.resetCurveControlValues();settle();
    auto expected=settings.currentPreset().keyCurve;
    int notifications=0;
    const auto connection=QObject::connect(&settings,&SettingsController::curveControlValuesChanged,
      [&]() { ++notifications; });
    QElapsedTimer burstTime;burstTime.start();
    int previous=-1;
    for(int i=0;i<512;++i) {
      const int phase=i%254;
      const int received=phase<=127 ? phase : 254-phase;
      if(previous>=0) setCurveParameter(expected,false,curveParameter(expected,false)+received-previous);
      previous=received;
      settings.previewCurveControlValue(0,received);
    }
    settle();settle();
    const auto actual=settings.currentPreset().keyCurve;
    check(std::abs(actual.x0-expected.x0)<1e-8 && std::abs(actual.x1-expected.x1)<1e-8,
          "Fast CC stream lost deltas or boundary reversals");
    check(notifications>0 && notifications<64,"CC burst generated excessive UI updates");
    QObject::disconnect(connection);
    std::cout<<"CC burst: "<<burstTime.elapsed()<<" ms, "<<notifications<<" UI notifications\n";
    window->grabWindow().save("curve-controls-ui.png");
    window->resize(1024,600);settle();
    for(int slot=0;slot<4;++slot) {
      auto* row=findVisualItem(window->contentItem(),"curveEditor"+QString::number(slot/2));
      auto* valueField=findVisualItem(window->contentItem(),"curveCcValue"+QString::number(slot));
      check(row && valueField,"Curve layout missing");
      const auto pos=valueField->mapToItem(row,QPointF(0,0));
      check(pos.y()>=0 && pos.y()+valueField->height()<=row->height(),"Curve fields overlap next panel at 1024x600");
    }
    window->grabWindow().save("curve-controls-ui-1024.png");
    window->resize(640,360);page->setProperty("singleCurveMode",true);settle();
    window->grabWindow().save("curve-controls-ui-phone.png");
    std::cout<<"Curve MIDI UI checks passed\n";
  } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
