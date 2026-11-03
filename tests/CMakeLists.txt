add_executable(MorphRoutingTests
  MidiEngineRoutingTest.cpp
  ../src/midi/MidiEngine.cpp ../src/midi/MidiEngine.h
  ../src/core/ExpressionCalculator.cpp ../src/core/PiecewiseCurve.cpp
)
target_link_libraries(MorphRoutingTests PRIVATE NaadaLab::Midi Qt6::Core)
add_test(NAME MorphRoutingTests COMMAND MorphRoutingTests)

add_executable(CurvePresetTests CurvePresetTest.cpp
  ../src/core/Presets.cpp ../src/core/Presets.h)
target_link_libraries(CurvePresetTests PRIVATE Qt6::Core)
add_test(NAME CurvePresetTests COMMAND CurvePresetTests)

set(CURVE_UI_SOURCES ${MORPHMASTER_SOURCES})
list(FILTER CURVE_UI_SOURCES EXCLUDE REGEX "src/main.cpp$")
list(TRANSFORM CURVE_UI_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/")
add_executable(CurveUiTests CurveUiTest.cpp ${CURVE_UI_SOURCES})
target_compile_definitions(CurveUiTests PRIVATE MORPH_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(CurveUiTests PRIVATE NaadaLab::Midi NaadaLab::Ui NaadaLab::Manual
  Qt6::Concurrent Qt6::Svg Qt6::Qml Qt6::Quick Qt6::QuickControls2)
add_test(NAME CurveUiTests COMMAND CurveUiTests)
get_target_property(CURVE_UI_QT_CORE Qt6::Core IMPORTED_LOCATION_DEBUG)
get_filename_component(CURVE_UI_QT_BIN "${CURVE_UI_QT_CORE}" DIRECTORY)
set_tests_properties(CurveUiTests PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QPA_PLATFORM_PLUGIN_PATH=${CURVE_UI_QT_BIN}/../plugins/platforms")
