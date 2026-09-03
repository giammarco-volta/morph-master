cmake_minimum_required(VERSION 3.21)
project(MorphMaster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
  Core
  Gui
  Widgets
  Concurrent
  Svg
  Qml
  Quick
  QuickControls2
)

qt_standard_project_setup()

add_subdirectory(
  "${CMAKE_CURRENT_SOURCE_DIR}/../Common"
  "${CMAKE_CURRENT_BINARY_DIR}/Common"
)

if(QT_KNOWN_POLICY_QTP0002)
    qt_policy(SET QTP0002 NEW)
endif()

set(MORPHMASTER_SOURCES
  src/main.cpp
  src/About.h

  src/core/ExpressionCalculator.h
  src/core/ExpressionCalculator.cpp
  src/core/MorphOutputId.h
  src/core/PiecewiseCurve.h
  src/core/PiecewiseCurve.cpp
  src/core/KeyboardStandardRanges.h
  src/core/Footage.h
  src/core/Presets.h
  src/core/Presets.cpp
  src/core/InstrumentDefinition.h
  src/core/InsParser.h
  src/core/InsParser.cpp
  src/core/InstrumentDatabase.h
  src/core/InstrumentDatabase.cpp
  src/core/JsonSerializer.h
  src/core/JsonSerializer.cpp

  src/controllers/SettingsController.h
  src/controllers/SettingsController.cpp
  src/controllers/TrackController.h
  src/controllers/TrackController.cpp
  src/controllers/CurveController.h
  src/controllers/CurveController.cpp
  src/controllers/MorphOutputStateModel.h
  src/controllers/MorphOutputStateModel.cpp

  ../Common/src/midi/MidiMessage.h

  ../Common/src/midi/IMidiOut.h
  ../Common/src/midi/MidiOutFactory.h
  ../Common/src/midi/IMidiIn.h
  ../Common/src/midi/MidiInFactory.h

  ../Common/src/midi/MidiMonoIn.h
  src/midi/MidiMonoIn.cpp
  src/midi/MidiEngine.h
  src/midi/MidiEngine.cpp
)

if(WIN32)
  list(APPEND MORPHMASTER_SOURCES
    ../Common/src/midi/MidiOut_WinMM.h
    ../Common/src/midi/MidiIn_WinMM.h
  )
elseif(ANDROID)
  list(APPEND MORPHMASTER_SOURCES
    ../Common/src/midi/MidiAndroidBridge.h
    ../Common/src/midi/MidiIn_Android.h
    ../Common/src/midi/MidiOut_Android.h
  )
else()
  list(APPEND MORPHMASTER_SOURCES
    ../Common/src/midi/MidiOut_Stub.h
    ../Common/src/midi/MidiIn_Stub.h
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
else()
  add_executable(MorphMaster
    ${MORPHMASTER_SOURCES}
  )
endif()

set_source_files_properties(
  src/qml/Theme.qml
  PROPERTIES
  QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(
  src/qml/UiMetrics.qml
  PROPERTIES
  QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(MorphMaster
  URI MorphMaster
  VERSION 1.0
  RESOURCE_PREFIX /qt/qml
  NO_RESOURCE_TARGET_PATH
  QML_FILES
    src/qml/Theme.qml
    src/qml/UiMetrics.qml
    src/qml/Main.qml

    src/qml/navigation/NavigationShell.qml

    src/qml/pages/MorphOutputAssignmentPage.qml
    src/qml/pages/MorphMonitorPage.qml
    src/qml/pages/TracksSettingsPage.qml
    src/qml/pages/CurvesSettingsPage.qml
    src/qml/pages/AboutPage.qml
    src/qml/pages/UserManualPage.qml

    src/qml/controls/LabeledControl.qml

    src/qml/controls/base/LabeledComboBox.qml
    src/qml/controls/base/SearchableComboField.qml
    src/qml/controls/base/ModalPanel.qml
    src/qml/controls/base/FormSection.qml
    src/qml/controls/base/ActionButton.qml
    src/qml/controls/base/DragValueField.qml

    src/qml/controls/midi/MidiChannelSelector.qml
    src/qml/controls/midi/PitchBendRangeSelector.qml
    src/qml/controls/midi/MidiKnob.qml

    src/qml/widgets/MidiConfiguration.qml
    src/qml/widgets/PerformanceSettings.qml
    src/qml/widgets/PresetSelector.qml
    src/qml/widgets/ConfigurationNode.qml
    src/qml/widgets/ConfigurationOverview.qml
    src/qml/widgets/CurveGraph.qml
    src/qml/widgets/CurveEditRow.qml
    src/qml/widgets/DraggableTrackTile.qml
    src/qml/widgets/MorphOutputAssignmentPanel.qml
    src/qml/widgets/MorphMonitorPanel.qml
    src/qml/widgets/MorphMonitorCurveGraph.qml
    src/qml/widgets/MorphMonitorCornerGraph.qml
    src/qml/widgets/MorphTestKeyboard.qml
    src/qml/widgets/MorphCornerSurfaceGraph.qml
    src/qml/widgets/SingleTrackEditor.qml
    src/qml/widgets/InstrumentProgramBrowserPopup.qml
    src/qml/widgets/SearchableSelectionPopup.qml
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/Main.qml")
  message(FATAL_ERROR "Missing src/qml/Main.qml")
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
  src/resources/manual/MorphoraUserManual.html
  PROPERTIES QT_RESOURCE_ALIAS "manual/MorphoraUserManual.html"
)

set_source_files_properties(
  src/resources/svg/gear.svg
  PROPERTIES QT_RESOURCE_ALIAS "svg/gear.svg"
)

file(GLOB MANUAL_IMAGE_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/manual/images/*.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/manual/images/*.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/manual/images/*.jpeg"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/manual/images/*.svg"
)

foreach(manual_image ${MANUAL_IMAGE_FILES})
  get_filename_component(manual_image_name "${manual_image}" NAME)

  set_source_files_properties(
    "${manual_image}"
    PROPERTIES
      QT_RESOURCE_ALIAS "manual/images/${manual_image_name}"
  )
endforeach()

qt_add_resources(MorphMaster MorphMasterDataResources
  PREFIX "/"
  FILES
    src/resources/qdarkstyle/darkstyle.qss
    src/resources/Ins/instrument_database.json
    src/resources/manual/MorphoraUserManual.html
    ${MANUAL_IMAGE_FILES}
    src/resources/svg/gear.svg
)

if(WIN32)
    target_sources(MorphMaster PRIVATE
        src/resources/windows/MorphMaster.rc
    )
endif()

target_link_libraries(MorphMaster PRIVATE
  NaadaLab::Midi
  Qt6::Concurrent
  Qt6::Widgets
  Qt6::Svg
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
)

if(WIN32)
  target_link_libraries(MorphMaster PRIVATE winmm)
endif()

if(ANDROID)
  set_property(TARGET MorphMaster PROPERTY
     QT_ANDROID_PACKAGE_SOURCE_DIR
     "${CMAKE_CURRENT_SOURCE_DIR}/android"
  )

  set_property(TARGET MorphMaster PROPERTY
     QT_ANDROID_PACKAGE_NAME
     "com.naadalab.morphmaster"
  )
endif()

qt_finalize_executable(MorphMaster)
