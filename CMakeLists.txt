cmake_minimum_required(VERSION 3.22)

project(SaturAir VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------
# JUCE
#
# Padrao: baixa automaticamente via FetchContent na primeira configuracao.
#   cmake -B build
#
# Ou aponte para uma copia local do JUCE:
#   cmake -B build -DJUCE_PATH="C:/JUCE"
#
# Ou deixe o find_package(JUCE CONFIG) encontrar uma instalacao.
# ------------------------------------------------------------------
if(DEFINED JUCE_PATH AND NOT JUCE_PATH STREQUAL "")
    add_subdirectory("${JUCE_PATH}" juce)
else()
    find_package(JUCE CONFIG QUIET)
    if(NOT JUCE_FOUND)
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        8.0.14
            GIT_SHALLOW    TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()
endif()

juce_add_plugin(SaturAir
    COMPANY_NAME                "CavalAudio"
    BUNDLE_ID                   "com.cavalaudio.saturair"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Satr
    PLUGIN_CODE                 Sair
    FORMATS                     VST3
    VST3_CATEGORIES             Fx Distortion
    PRODUCT_NAME                "SaturAir")

target_sources(SaturAir
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(SaturAir
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SaturAir
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Fontes UTF-8 com acentos (nomes de UI em portugues): evita C4819 no MSVC.
if(MSVC)
    target_compile_options(SaturAir PRIVATE /utf-8)
endif()
