# ============================================================================
#  AudioFX - System-Wide Audio Effects Processor
#  JUCE 8 + CMake  ->  Visual Studio 18 (2026) / v145 toolset
#
#  Configure :  cmake -B build -G "Visual Studio 18 2026" -A x64
#  Build     :  cmake --build build --config Release
# ============================================================================
cmake_minimum_required(VERSION 3.22)

project(AudioFX VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------ JUCE ----
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# --------------------------------------------------------- binary assets ----
juce_add_binary_data(AudioFXAssets
    NAMESPACE BinaryData
    SOURCES resources/logo.png)

# ------------------------------------------------------------- the app ------
juce_add_gui_app(AudioFX
    PRODUCT_NAME                "AudioFX"
    COMPANY_NAME                "AudioFX"
    BUNDLE_ID                   "com.audiofx.desktop"
    DESCRIPTION                 "System-wide audio FX: 10-band EQ, spectrum, reverb, field surround, limiter, master/AGC"
    VERSION                     ${PROJECT_VERSION}
    # NOTE: JUCE CMake only accepts .png here (it generates the .ico itself).
    # Passing .ico hits JUCE bug #1547 ("Could not find icon file .../icon.ico").
    ICON_BIG                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.png"
    ICON_SMALL                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.png"
    MICROPHONE_PERMISSION_ENABLED TRUE)

# generates <JuceHeader.h> (included throughout Source/) into the artefacts dir
juce_generate_juce_header(AudioFX)

target_sources(AudioFX PRIVATE
    Source/Main.cpp
    Source/AudioEngine.cpp
    Source/State/PresetManager.cpp
    Source/UI/LookAndFeel.cpp
    Source/UI/Controls.cpp
    Source/UI/SpectrumComponent.cpp
    Source/UI/EQPanel.cpp
    Source/UI/ModulePanels.cpp
    Source/UI/DeviceBar.cpp
    Source/UI/MainComponent.cpp)

target_include_directories(AudioFX PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(AudioFX PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_ASIO=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0
    JUCE_ALSA=0
    JUCE_JACK=0
    _CRT_SECURE_NO_WARNINGS)

target_link_libraries(AudioFX PRIVATE
    AudioFXAssets
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_gui_extra
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if(WIN32)
    target_link_libraries(AudioFX PRIVATE ole32 avrt uuid winmm)
endif()
