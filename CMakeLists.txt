cmake_minimum_required(VERSION 3.21)
project(MorphMaster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Pdf PdfWidgets Concurrent Svg)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0002)
    qt_policy(SET QTP0002 NEW)
endif()

set(MORPHMASTER_SOURCES
  src/main.cpp
  src/MainWindow.h
  src/MainWindow.cpp
  src/StyleUtils.h
  src/StyleUtils.cpp

  src/core/About.h
  src/core/Types.h
  src/core/MorphCurve.h
  src/core/ExpressionCalculator.h
  src/core/ExpressionCalculator.cpp
  src/core/ExpressionCurveId.h
  src/core/TrackGroupId.h
  src/core/GroupPlacement.h
  src/core/GroupUtils.h
  src/core/PiecewiseCurve.h
  src/core/PiecewiseCurve.cpp
  src/core/KeyboardStandardRanges.h
  src/core/Footage.h
  src/core/Presets.h
  src/core/Presets.cpp
  src/core/InstrumentDefinition.h
  src/core/CategoryClassifier.h
  src/core/CategoryClassifier.cpp
  src/core/InsParser.h
  src/core/InsParser.cpp
  src/core/InstrumentDatabase.h
  src/core/InstrumentDatabase.cpp
  src/core/JsonSerializer.h
  src/core/JsonSerializer.cpp

  ../Common/src/midi/MidiMessage.h

  ../Common/src/midi/IMidiOut.h
  ../Common/src/midi/MidiOutFactory.h
  ../Common/src/midi/MidiOutFactory.cpp
  ../Common/src/midi/IMidiIn.h
  ../Common/src/midi/MidiInFactory.h
  ../Common/src/midi/MidiInFactory.cpp

  ../Common/src/midi/MidiMonoIn.h
  src/midi/MidiMonoIn.cpp

  src/ui/tabs/MidiSettingsTab.h
  src/ui/tabs/MidiSettingsTab.cpp
  src/ui/tabs/SurfaceTab.h
  src/ui/tabs/SurfaceTab.cpp
  src/ui/tabs/CurveEditorTab.h
  src/ui/tabs/CurveEditorTab.cpp
  src/ui/tabs/FourTracksTab.h
  src/ui/tabs/FourTracksTab.cpp

  ../Common/src/ui/widgets/MidiValueSelector.h
  ../Common/src/ui/widgets/MidiValueSelector.cpp
  ../Common/src/ui/widgets/MidiChannelSelector.h
  ../Common/src/ui/widgets/MidiChannelSelector.cpp
  ../Common/src/ui/widgets/ManualWidget.h
  ../Common/src/ui/widgets/ManualWidget.cpp
  ../Common/src/ui/widgets/TouchComboBox.h
  ../Common/src/ui/widgets/TouchComboBox.cpp
  src/ui/widgets/TrackStripWidget.h
  src/ui/widgets/TrackStripWidget.cpp
  src/ui/widgets/MorphSurfaceWidget.h
  src/ui/widgets/MorphSurfaceWidget.cpp
  src/ui/widgets/CurveEditorWidget.h
  src/ui/widgets/CurveEditorWidget.cpp
  src/ui/widgets/CurveRowWidget.h
  src/ui/widgets/CurveRowWidget.cpp
)

if(WIN32)
  list(APPEND MORPHMASTER_SOURCES
    ../Common/src/midi/MidiOut_WinMM.h
    ../Common/src/midi/MidiOut_WinMM.cpp
    ../Common/src/midi/MidiIn_WinMM.h
    ../Common/src/midi/MidiIn_WinMM.cpp
  )
elseif(ANDROID)
  list(APPEND MORPHMASTER_SOURCES
    ../Common/src/midi/MidiAndroidBridge.h
    ../Common/src/midi/MidiAndroidBridge.cpp
    ../Common/src/midi/MidiIn_Android.h
    ../Common/src/midi/MidiIn_Android.cpp
    ../Common/src/midi/MidiOut_Android.h
    ../Common/src/midi/MidiOut_Android.cpp
  )
else()
  list(APPEND MORPHMASTER_SOURCES
    ../Common/src/midi/MidiOut_Stub.h
    ../Common/src/midi/MidiOut_Stub.cpp
    ../Common/src/midi/MidiIn_Stub.h
    ../Common/src/midi/MidiIn_Stub.cpp
  )
endif()

if(ANDROID)
  set(QT_ANDROID_COMPILE_SDK_VERSION 34)
  set(QT_ANDROID_MIN_SDK_VERSION 23)
  set(QT_ANDROID_TARGET_SDK_VERSION 34)
  
  qt_add_executable(MorphMaster
    MANUAL_FINALIZATION
    ${MORPHMASTER_SOURCES}
  )
  set_property(TARGET MorphMaster PROPERTY
     QT_ANDROID_PACKAGE_SOURCE_DIR
     "${CMAKE_CURRENT_SOURCE_DIR}/android"
  )
else()
  add_executable(MorphMaster
    ${MORPHMASTER_SOURCES}
  )
endif()

set_source_files_properties(
  src/resources/qdarkstyle/darkstyle.qss
  PROPERTIES QT_RESOURCE_ALIAS "qdarkstyle/darkstyle.qss"
)

set_source_files_properties(
  src/resources/Ins/instrument_database.json
  PROPERTIES QT_RESOURCE_ALIAS "Ins/instrument_database.json"
)

set_source_files_properties(
  src/resources/manual/MorphMasterUserManual.pdf
  PROPERTIES QT_RESOURCE_ALIAS "manual/MorphMasterUserManual.pdf"
)

set_source_files_properties(
  src/resources/svg/bass-clef.svg
  PROPERTIES QT_RESOURCE_ALIAS "svg/bass-clef.svg"
)

set_source_files_properties(
  src/resources/svg/treble-clef.svg
  PROPERTIES QT_RESOURCE_ALIAS "svg/treble-clef.svg"
)

message(STATUS "bass svg exists: ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/svg/bass-clef.svg")
message(STATUS "treble svg exists: ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/svg/treble-clef.svg")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/svg/bass-clef.svg")
  message(FATAL_ERROR "Missing bass-clef.svg")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/svg/treble-clef.svg")
  message(FATAL_ERROR "Missing treble-clef.svg")
endif()

qt_add_resources(MorphMaster MorphMasterDataResources
  PREFIX "/"
  FILES
    src/resources/qdarkstyle/darkstyle.qss
    src/resources/Ins/instrument_database.json
    src/resources/manual/MorphMasterUserManual.pdf
    src/resources/svg/bass-clef.svg
    src/resources/svg/treble-clef.svg
)

target_link_libraries(MorphMaster PRIVATE Qt6::Concurrent Qt6::Widgets Qt6::Pdf Qt6::PdfWidgets Qt6::Svg)

if(WIN32)
  target_link_libraries(MorphMaster PRIVATE winmm)
endif()

qt_finalize_executable(MorphMaster)
