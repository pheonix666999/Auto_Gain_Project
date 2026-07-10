cmake_minimum_required(VERSION 3.22)

project(WapDemSaturation VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WAPDEM_ENABLE_AAX "Build the AAX plugin format" OFF)
set(WAPDEM_AAX_SDK_PATH "" CACHE PATH "Path to the AAX SDK root")

# ----------------------------------------------------------------------------
# JUCE
# ----------------------------------------------------------------------------
# Option A (recommended): place JUCE as a submodule/folder next to this file.
#   git clone https://github.com/juce-framework/JUCE.git
# Option B: let CMake fetch it (uncomment the FetchContent block below).
# ----------------------------------------------------------------------------

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4          # pin to a known-good release
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ----------------------------------------------------------------------------
# Plugin target
# ----------------------------------------------------------------------------
# NOTE on formats:
#   VST3 / Standalone build out of the box.
#   AU is added automatically on macOS.
#   AAX requires the Avid AAX SDK and must be enabled explicitly.
# ----------------------------------------------------------------------------

set(WAPDEM_PLUGIN_FORMATS VST3 Standalone)

if(APPLE)
    list(APPEND WAPDEM_PLUGIN_FORMATS AU)
endif()

set(WAPDEM_PLUGIN_EXTRA_ARGS)

if(WAPDEM_ENABLE_AAX)
    if(WAPDEM_AAX_SDK_PATH STREQUAL "")
        message(FATAL_ERROR "WAPDEM_ENABLE_AAX=ON requires -DWAPDEM_AAX_SDK_PATH=/path/to/AAX/SDK")
    endif()

    juce_set_aax_sdk_path("${WAPDEM_AAX_SDK_PATH}")
    list(APPEND WAPDEM_PLUGIN_FORMATS AAX)
    list(APPEND WAPDEM_PLUGIN_EXTRA_ARGS
        AAX_CATEGORY AAX_ePlugInCategory_Harmonic
    )
endif()

juce_add_plugin(WapDemSaturation
    COMPANY_NAME              "Tractone Audio"
    PLUGIN_MANUFACTURER_CODE  Trtn
    PLUGIN_CODE               Wapd
    FORMATS                   ${WAPDEM_PLUGIN_FORMATS}
    PRODUCT_NAME              "WapDem Saturation"
    BUNDLE_ID                 com.tractoneaudio.wapdemsaturation
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Fx Distortion
    VST3_AUTO_MANIFEST        FALSE
    AU_MAIN_TYPE              kAudioUnitType_Effect
    ${WAPDEM_PLUGIN_EXTRA_ARGS}
)

juce_add_binary_data(WapDemSaturation_Data
    HEADER_NAME "BinaryData.h"
    NAMESPACE "BinaryData"
    SOURCES
        Resources/background.png
        Resources/logo.png
)

target_sources(WapDemSaturation PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(WapDemSaturation PRIVATE Source)

target_compile_definitions(WapDemSaturation PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(WapDemSaturation PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    WapDemSaturation_Data
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
