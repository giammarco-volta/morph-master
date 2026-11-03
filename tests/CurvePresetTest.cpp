#include "../src/core/Presets.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include <stdexcept>

static void check(bool condition,const char* message) {
  if(!condition) throw std::runtime_error(message);
}
int main(int argc,char** argv) {
  QCoreApplication app(argc,argv);
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("MorphoraTests");
  QCoreApplication::setApplicationName("CurveControls-"+QUuid::createUuid().toString(QUuid::WithoutBraces));
  const QString directory=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  const QString path=QDir(directory).filePath("presets.json");
  int result=0;
  try {
    PresetManager manager;
    PresetData preset; preset.name="Curve test";
    for(int i=0;i<4;++i) preset.curveControls[i]={7,double(10+i),double(90+i),i+2};
    preset.keyCurve.x0=32.25; preset.keyCurve.x1=95.75;
    check(manager.savePreset(preset),"Could not save isolated test preset");
    PresetManager loaded;
    check(loaded.loadFromDisk(),"Could not reload preset");
    const auto* restored=loaded.findPreset(preset.name);
    check(restored!=nullptr,"Saved preset missing");
    check(restored->keyCurve.x0==32.25 && restored->keyCurve.x1==95.75,"Fractional endpoints lost on save");
    for(int i=0;i<4;++i) {
      const auto& a=restored->curveControls[i]; const auto& b=preset.curveControls[i];
      check(a.cc==b.cc && a.channel==b.channel && a.minimum==b.minimum && a.maximum==b.maximum,"Curve assignment round-trip failed");
    }
    QFile file(path); check(file.open(QIODevice::ReadOnly),"Could not inspect test preset");
    auto root=QJsonDocument::fromJson(file.readAll()).object(); file.close();
    auto saved=root["presets"].toArray()[0].toObject();
    saved.remove("curveControls");
    auto oldSettings=saved["appInitSettings"].toObject();
    oldSettings["followExtremeNotes"]=true; saved["appInitSettings"]=oldSettings;
    auto oldOutputs=saved["morphOutputs"].toArray();
    for(int i : {2,6}) { auto output=oldOutputs[i].toObject(); output["extremeNoteOnly"]=true; oldOutputs[i]=output; }
    saved["morphOutputs"]=oldOutputs;
    root["presets"]=QJsonArray{saved};
    check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"Could not write legacy fixture");
    file.write(QJsonDocument(root).toJson()); file.close();
    check(loaded.loadFromDisk(),"Legacy preset failed to load");
    restored=loaded.findPreset(preset.name);
    for(const auto& a:restored->curveControls) check(a.cc==-1,"Legacy preset must default to None");
    check(loaded.saveToDisk(),"Could not migrate legacy preset");
    check(file.open(QIODevice::ReadOnly),"Could not inspect migrated preset");
    const auto migrated=QJsonDocument::fromJson(file.readAll()).object()["presets"].toArray()[0].toObject();file.close();
    check(!migrated["appInitSettings"].toObject().contains("followExtremeNotes"),"Removed Follow persisted");
    for(const auto& output:migrated["morphOutputs"].toArray())
      check(!output.toObject().contains("extremeNoteOnly"),"Removed dedicated mode persisted");
    saved["curveControls"]=QJsonArray{
      QJsonObject{{"cc",11},{"minimum",-1},{"maximum",200}},
      QJsonObject{{"cc",20},{"minimum",0},{"maximum",0}},
      QJsonObject{{"cc",20}}, QJsonObject{{"cc",127}}};
    root["presets"]=QJsonArray{saved};
    check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"Could not write invalid fixture");
    file.write(QJsonDocument(root).toJson());file.close();
    check(loaded.loadFromDisk(),"Invalid mappings prevented preset loading");
    restored=loaded.findPreset(preset.name);
    check(restored->curveControls[0].cc==-1 && restored->curveControls[2].cc==-1
          && restored->curveControls[3].cc==-1 && restored->curveControls[1].minimum==1,
          "Reserved/duplicate mappings or zero-width ranges were not sanitized");
    std::cout<<"Curve preset persistence tests passed\n";
  } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';result=1; }
  // Only the unique test file and its empty directory; never recursive deletion.
  QFile::remove(path);
  QDir().rmdir(directory);
  return result;
}
