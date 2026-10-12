cmake_minimum_required(VERSION 3.24)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Supported product architecture")
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum product macOS")
project(nitride VERSION 0.7.0 LANGUAGES C CXX)

if(NOT APPLE)
    message(FATAL_ERROR "nitride currently targets macOS only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR NOT CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL "15.0")
    message(FATAL_ERROR "Supported Nitride profile is arm64 / macOS 15.0 deployment")
endif()
option(NITRIDE_BUILD_TESTS "Build local regression, safety and measurement tools" ON)
option(NITRIDE_BUILD_STUDIES "Build historical study applications" ON)
set(NITRIDE_SANITIZER "" CACHE STRING "Local diagnostic profile: address or thread")
if(NITRIDE_SANITIZER)
    if(NOT NITRIDE_SANITIZER MATCHES "^(address|thread)$")
        message(FATAL_ERROR "Unknown sanitizer")
    endif()
    add_compile_options("-fsanitize=${NITRIDE_SANITIZER}" -fno-omit-frame-pointer)
    add_link_options("-fsanitize=${NITRIDE_SANITIZER}")
endif()

include(cmake/JuceDependency.cmake)

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
    Source/Instrument/InstrumentSession.cpp
    Source/Instrument/PresetStore.cpp
    Source/Instrument/PitchMotionPanel.cpp
    Source/Instrument/InstrumentPanel.cpp
    Source/Instrument/MotionPanel.cpp
    Source/Instrument/FmPanel.cpp
)
target_include_directories(nitride PRIVATE Source)
target_compile_definitions(nitride PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(nitride PRIVATE
    nitride_dsp juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags
)

add_library(nitride_dsp STATIC Source/DSP/Engine.cpp Source/DSP/Fm.cpp)
add_library(nitride_study_engine ALIAS nitride_dsp)
target_include_directories(nitride_dsp PUBLIC Source/DSP Source/Studies)
# Keep the real-time DSP optimized while the native UI remains a normal Debug build.
target_compile_options(nitride_dsp PRIVATE $<$<CONFIG:Debug>:-O2>)
target_link_libraries(nitride_dsp PUBLIC juce::juce_recommended_warning_flags)

if(NITRIDE_BUILD_STUDIES)
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
endif()

juce_add_gui_app(nitride_review
    PRODUCT_NAME "Nitride Instrument Review"
    COMPANY_NAME "nitride"
    BUNDLE_ID "com.nitride.audio.review"
)
target_sources(nitride_review PRIVATE
    Source/Instrument/ReviewStandalone.cpp
    Source/Instrument/InstrumentRenderer.cpp
    Source/Instrument/InstrumentView.cpp
    Source/Instrument/InstrumentSession.cpp
    Source/Instrument/PresetStore.cpp
    Source/Instrument/PitchMotionPanel.cpp
    Source/Instrument/InstrumentPanel.cpp
    Source/Instrument/MotionPanel.cpp
    Source/Instrument/FmPanel.cpp
)
target_compile_definitions(nitride_review PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(nitride_review PRIVATE nitride_study_engine juce::juce_audio_utils
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

if(NITRIDE_BUILD_STUDIES)
add_executable(nitride_study_render Source/Studies/RenderStudies.cpp)
target_link_libraries(nitride_study_render PRIVATE nitride_study_engine)
endif()

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
    juce_add_console_app(nitride_automation_tests PRODUCT_NAME "nitride_automation_tests")
    target_sources(nitride_automation_tests PRIVATE Source/Tests/AutomationStateTests.cpp Source/Tests/FilterEnvelopeTests.cpp Source/Tests/LfoTests.cpp Source/Tests/FilterModulationTests.cpp Source/Tests/FmTests.cpp)
    target_include_directories(nitride_automation_tests PRIVATE Source)
    target_compile_definitions(nitride_automation_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    add_dependencies(nitride_automation_tests nitride)
    target_link_libraries(nitride_automation_tests PRIVATE "$<TARGET_FILE:nitride>" nitride_study_engine juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME nitride_automation_tests COMMAND nitride_automation_tests)
    juce_add_console_app(nitride_pitch_tests PRODUCT_NAME "nitride_pitch_tests")
    target_sources(nitride_pitch_tests PRIVATE Source/Tests/PitchMotionTests.cpp)
    target_include_directories(nitride_pitch_tests PRIVATE Source)
    target_compile_definitions(nitride_pitch_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    add_dependencies(nitride_pitch_tests nitride)
    target_link_libraries(nitride_pitch_tests PRIVATE "$<TARGET_FILE:nitride>" nitride_study_engine juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME nitride_pitch_tests COMMAND nitride_pitch_tests)
    juce_add_console_app(nitride_reference_tests PRODUCT_NAME "nitride_reference_tests")
    target_sources(nitride_reference_tests PRIVATE Source/Tests/CurrentRegressionTests.cpp)
    target_include_directories(nitride_reference_tests PRIVATE Source)
    target_compile_definitions(nitride_reference_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 NITRIDE_AUDIO_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/Source/Tests/Fixtures/Audio")
    add_dependencies(nitride_reference_tests nitride)
    target_link_libraries(nitride_reference_tests PRIVATE "$<TARGET_FILE:nitride>" nitride_study_engine juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME nitride_reference_tests COMMAND nitride_reference_tests)
    juce_add_console_app(nitride_safety_tests PRODUCT_NAME "nitride_safety_tests")
    target_sources(nitride_safety_tests PRIVATE Source/Tests/RealtimeSafetyTests.cpp)
    target_include_directories(nitride_safety_tests PRIVATE Source)
    target_compile_definitions(nitride_safety_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    if(NOT NITRIDE_SANITIZER)
        add_library(nitride_allocation_audit SHARED Source/Tests/AllocationAudit.cpp)
        target_compile_definitions(nitride_allocation_audit PRIVATE NITRIDE_ALLOCATION_AUDIT=1)
        target_link_libraries(nitride_safety_tests PRIVATE nitride_allocation_audit)
        target_compile_definitions(nitride_safety_tests PRIVATE NITRIDE_ALLOCATION_AUDIT=1)
    endif()
    add_dependencies(nitride_safety_tests nitride)
    target_link_libraries(nitride_safety_tests PRIVATE "$<TARGET_FILE:nitride>" nitride_study_engine juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME nitride_safety_tests COMMAND nitride_safety_tests)
    set_tests_properties(nitride_safety_tests PROPERTIES TIMEOUT 120)
    juce_add_console_app(nitride_perf PRODUCT_NAME "nitride_perf")
    target_sources(nitride_perf PRIVATE Source/Tests/PerformanceHarness.cpp)
    target_include_directories(nitride_perf PRIVATE Source)
    target_compile_definitions(nitride_perf PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 NITRIDE_BENCH_CONFIG="$<CONFIG>")
    add_dependencies(nitride_perf nitride)
    target_link_libraries(nitride_perf PRIVATE "$<TARGET_FILE:nitride>" nitride_study_engine juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_executable(nitride_study_tests Source/Studies/StudyTests.cpp)
    target_link_libraries(nitride_study_tests PRIVATE nitride_study_engine)
    add_test(NAME nitride_study_tests COMMAND nitride_study_tests)
    add_executable(nitride_au_component_tests Source/Tests/AuComponentTests.cpp)
    target_link_libraries(nitride_au_component_tests PRIVATE "-framework AudioToolbox" "-framework CoreFoundation")
endif()
