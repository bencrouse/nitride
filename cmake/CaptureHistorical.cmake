# Explicit fixture-production helper, never included in ordinary validation.
juce_add_console_app(nitride_history PRODUCT_NAME "nitride_history")
if(NOT NITRIDE_CAPTURE_SOURCE)
    message(FATAL_ERROR "Pass NITRIDE_CAPTURE_SOURCE pointing to CaptureHistorical.cpp")
endif()
target_sources(nitride_history PRIVATE "${NITRIDE_CAPTURE_SOURCE}")
target_include_directories(nitride_history PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_definitions(nitride_history PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    NITRIDE_LEGACY_VERSION=${PROJECT_VERSION_MINOR} NITRIDE_LEGACY_PRESETS=$<BOOL:$<EQUAL:${PROJECT_VERSION_MINOR},2>>)
add_dependencies(nitride_history nitride)
target_link_libraries(nitride_history PRIVATE "$<TARGET_FILE:nitride>" nitride_study_engine juce::juce_audio_utils juce::juce_recommended_config_flags)
