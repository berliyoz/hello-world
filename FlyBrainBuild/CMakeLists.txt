cmake_minimum_required(VERSION 3.22)
project(FlyBrainSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 9.0.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(FlyBrainSynth
  COMPANY_NAME "A Plus Experimental"
  BUNDLE_ID "best.aplus.flybrainsynth"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE Apls
  PLUGIN_CODE FbS1
  FORMATS VST3 Standalone
  PRODUCT_NAME "FlyBrain Synth"
)

juce_generate_juce_header(FlyBrainSynth)
target_sources(FlyBrainSynth PRIVATE Source/Plugin.cpp)
target_compile_definitions(FlyBrainSynth PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(FlyBrainSynth
  PRIVATE juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
