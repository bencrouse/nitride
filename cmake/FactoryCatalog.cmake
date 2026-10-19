# Adapt a generated copy, not the pinned vendor checkout. Only AU preset visibility
# and outer display labels change; the full historical program API stays intact.
get_target_property(nitride_module_path juce::juce_audio_plugin_client INTERFACE_JUCE_MODULE_PATH)
set(nitride_au_wrapper "${nitride_module_path}/juce_audio_plugin_client/juce_audio_plugin_client_AU_1.mm")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_factory_catalog.py"
    --wrapper "${nitride_au_wrapper}" --output "${CMAKE_CURRENT_BINARY_DIR}/generated"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RESULT_VARIABLE catalog_result)
if(NOT catalog_result EQUAL 0)
    message(FATAL_ERROR "Factory catalog/AU adapter generation failed")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/presets/factory/candidate-001.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/presets/factory/originals.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_factory_catalog.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/factory_presets.py")
set(nitride_wrapper_replacements 0)
foreach(wrapper_target juce_audio_plugin_client juce_audio_plugin_client_AU)
    get_target_property(wrapper_sources ${wrapper_target} INTERFACE_SOURCES)
    if(wrapper_sources)
        string(FIND "${wrapper_sources}" "${nitride_au_wrapper}" wrapper_position)
        if(NOT wrapper_position EQUAL -1)
            string(REPLACE "${nitride_au_wrapper}" "${CMAKE_CURRENT_BINARY_DIR}/generated/NitrideAuWrapper.mm" wrapper_sources "${wrapper_sources}")
            set_property(TARGET ${wrapper_target} PROPERTY INTERFACE_SOURCES "${wrapper_sources}")
            math(EXPR nitride_wrapper_replacements "${nitride_wrapper_replacements}+1")
        endif()
    endif()
endforeach()
if(nitride_wrapper_replacements EQUAL 0)
    message(FATAL_ERROR "Pinned AU source target not found; do not silently ship an unfiltered legacy menu")
endif()
