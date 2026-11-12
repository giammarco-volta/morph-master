#include "../src/controllers/SettingsController.h"
#include "../src/controllers/CurveController.h"
#include "../src/controllers/TrackController.h"
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
    auto* keyGraph=findVisualItem(window->contentItem(),"curveGraph0");
    auto* velocityGraph=findVisualItem(window->contentItem(),"curveGraph1");
    check(keyGraph && velocityGraph, "Curve feedback graphs missing");
    const QVariantList heldNotes{QVariantMap{{"note",60},{"velocity",90},{"outputMask",255}}};
    emit settings.monitorFeedbackChanged(heldNotes, {});
    settle();
    check(keyGraph->property("feedbackNotes").toList().size()==1
          && velocityGraph->property("feedbackNotes").toList().size()==1,
          "Played notes did not reach both curve editors");
    emit settings.monitorFeedbackChanged({}, {});
    settle();
    check(keyGraph->property("feedbackNotes").toList().isEmpty()
          && velocityGraph->property("feedbackNotes").toList().isEmpty(),
          "Released notes remained on curve editors");
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
    // The first assigned track owns the automatic name until it leaves.
    auto* first = qobject_cast<TrackController*>(settings.track(5));
    auto* second = qobject_cast<TrackController*>(settings.track(2));
    check(first && second, "Track controllers missing");
    first->setProgramNumber(40);
    second->setProgramNumber(41);
    first->setMorphOutput(0);
    auto& output = settings.currentPreset().morphOutputs[0];
    const auto firstName = output.name;
    check(!firstName.isEmpty() && output.nameTrackIndex == 4 && !output.customName,
          "First assignment did not establish automatic name");
    second->setMorphOutput(0);
    second->setProgramNumber(42);
    check(output.name == firstName && output.nameTrackIndex == 4,
          "Second track replaced the first automatic name");
    first->setProgramNumber(43);
    check(output.name != firstName, "First track program did not update automatic name");
    settings.setMorphOutputName(0, "My layered sound");
    first->setProgramNumber(44);
    check(output.name == "My layered sound" && output.customName,
          "Program change replaced custom name");
    first->setMorphOutput(8);
    check(output.name == "My layered sound" && output.nameTrackIndex == 1,
          "Removing owner lost custom name or remaining track");
    second->setMorphOutput(8);
    check(output.name.isEmpty() && !output.customName && output.nameTrackIndex == -1,
          "Empty output retained its previous name");
    second->setMorphOutput(0);
    check(!output.name.isEmpty() && !output.customName && output.nameTrackIndex == 1,
          "Reassignment did not initialize a fresh name");
    second->reset();
    check(output.name.isEmpty() && !output.customName, "Track reset left a stale output name");
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
