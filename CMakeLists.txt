cmake_minimum_required(VERSION 3.22)
project(VocalEnhancePro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(VocalEnhancePro
    COMPANY_NAME "VocalEnhance"
    PLUGIN_MANUFACTURER_CODE Venh
    PLUGIN_CODE Vepr
    FORMATS VST3
    PRODUCT_NAME "VocalEnhancePro"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Fx
)

juce_generate_juce_header(VocalEnhancePro)

target_sources(VocalEnhancePro
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(VocalEnhancePro
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(VocalEnhancePro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
