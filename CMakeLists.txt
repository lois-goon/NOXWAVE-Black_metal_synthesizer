cmake_minimum_required(VERSION 3.22)
project(NoxWave VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
endif()

option(NOX_BUILD_TESTS "Build the offline render test tool" OFF)

# --- JUCE : dossier local ./JUCE s'il existe, sinon téléchargement automatique ---
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(NOX_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND NOX_FORMATS AU)
endif()

juce_add_plugin(NoxWave
    COMPANY_NAME "NoxAudio"
    PLUGIN_MANUFACTURER_CODE Noxa
    PLUGIN_CODE Nxwv
    FORMATS ${NOX_FORMATS}
    PRODUCT_NAME "NoxWave"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

set(NOX_SOURCES
    Source/Params.cpp
    Source/Wavetable.cpp
    Source/Engine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(NoxWave PRIVATE ${NOX_SOURCES})

set(NOX_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_compile_definitions(NoxWave PUBLIC ${NOX_DEFS})

target_link_libraries(NoxWave
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (NOX_BUILD_TESTS)
    juce_add_console_app(NoxRenderTest PRODUCT_NAME "NoxRenderTest")
    target_sources(NoxRenderTest PRIVATE Tests/RenderTest.cpp ${NOX_SOURCES})
    target_compile_definitions(NoxRenderTest PRIVATE ${NOX_DEFS} JucePlugin_Name="NoxWave"
        JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0)
    target_include_directories(NoxRenderTest PRIVATE Source)
    target_link_libraries(NoxRenderTest PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
