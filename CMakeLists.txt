cmake_minimum_required(VERSION 3.24)
project(nitride VERSION 0.1.0 LANGUAGES C CXX)

if(NOT APPLE)
    message(FATAL_ERROR "nitride currently targets macOS only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "" FORCE)
option(NITRIDE_BUILD_TESTS "Build native UI/state smoke checks." ON)

set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../carbide/JUCE" CACHE PATH "JUCE 8 checkout")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Set JUCE_DIR to your JUCE 8 checkout.")
endif()
add_subdirectory("${JUCE_DIR}" JUCE)

juce_add_plugin(nitride
    COMPANY_NAME "nitride"
    BUNDLE_ID "com.nitride.audio.nitride"
    PLUGIN_MANUFACTURER_CODE NTRD
    PLUGIN_CODE NT01
    FORMATS AU Standalone
    PRODUCT_NAME "nitride"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(nitride PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Instrument/InstrumentRenderer.cpp
    Source/Instrument/InstrumentView.cpp
)
target_include_directories(nitride PRIVATE Source)
target_compile_definitions(nitride PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(nitride PRIVATE
    nitride_study_engine juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags
)

add_library(nitride_study_engine STATIC Source/Studies/StudyEngine.cpp)
target_include_directories(nitride_study_engine PUBLIC Source/Studies)
# Keep the real-time DSP optimized while the native UI remains a normal Debug build.
target_compile_options(nitride_study_engine PRIVATE $<$<CONFIG:Debug>:-O2>)
target_link_libraries(nitride_study_engine PUBLIC juce::juce_recommended_warning_flags)

juce_add_gui_app(nitride_studies
    PRODUCT_NAME "Nitride Sound Studies"
    COMPANY_NAME "nitride"
    BUNDLE_ID "com.nitride.audio.studies"
)
target_sources(nitride_studies PRIVATE Source/Studies/StudyApplication.cpp)
target_compile_definitions(nitride_studies PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(nitride_studies PRIVATE nitride_study_engine juce::juce_audio_utils
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_gui_app(nitride_gestures
    PRODUCT_NAME "Nitride Gesture Study"
    COMPANY_NAME "nitride"
    BUNDLE_ID "com.nitride.audio.gestures"
)
target_sources(nitride_gestures PRIVATE Source/Studies/GestureApplication.cpp)
target_compile_definitions(nitride_gestures PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(nitride_gestures PRIVATE nitride_study_engine juce::juce_audio_utils
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_gui_app(nitride_deformation
    PRODUCT_NAME "Nitride Deformation Study"
    COMPANY_NAME "nitride"
    BUNDLE_ID "com.nitride.audio.deformation"
)
target_sources(nitride_deformation PRIVATE Source/Studies/GestureApplication.cpp)
target_compile_definitions(nitride_deformation PRIVATE NITRIDE_DEFORMATION_STUDY=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(nitride_deformation PRIVATE nitride_study_engine juce::juce_audio_utils
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

juce_add_gui_app(nitride_review
    PRODUCT_NAME "Nitride Instrument Review"
    COMPANY_NAME "nitride"
    BUNDLE_ID "com.nitride.audio.review"
)
target_sources(nitride_review PRIVATE
    Source/Instrument/ReviewStandalone.cpp
    Source/Instrument/InstrumentRenderer.cpp
    Source/Instrument/InstrumentView.cpp
)
target_compile_definitions(nitride_review PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(nitride_review PRIVATE nitride_study_engine juce::juce_audio_utils
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

add_executable(nitride_study_render Source/Studies/RenderStudies.cpp)
target_link_libraries(nitride_study_render PRIVATE nitride_study_engine)

if(NITRIDE_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(nitride_ui_tests PRODUCT_NAME "nitride_ui_tests")
    target_sources(nitride_ui_tests PRIVATE Source/Tests/UiSelfTests.cpp)
    target_include_directories(nitride_ui_tests PRIVATE Source)
    target_compile_definitions(nitride_ui_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    # Reuse the actual plugin code without inheriting its plugin-wrapper definitions.
    add_dependencies(nitride_ui_tests nitride)
    target_link_libraries(nitride_ui_tests PRIVATE "$<TARGET_FILE:nitride>" nitride_study_engine juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME nitride_ui_tests COMMAND nitride_ui_tests)
    add_executable(nitride_study_tests Source/Studies/StudyTests.cpp)
    target_link_libraries(nitride_study_tests PRIVATE nitride_study_engine)
    add_test(NAME nitride_study_tests COMMAND nitride_study_tests)
    add_executable(nitride_au_component_tests Source/Tests/AuComponentTests.cpp)
    target_link_libraries(nitride_au_component_tests PRIVATE "-framework AudioToolbox")
endif()
