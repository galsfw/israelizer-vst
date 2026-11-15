# Testes unitarios do SaturAir (Catch2 v3).
# Fora do build padrao: ative com -DSATURAIR_BUILD_TESTS=ON e rode com
#   ctest --test-dir build -C Release --output-on-failure

include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1)
FetchContent_MakeAvailable(Catch2)

add_executable(SaturAirTests SaturAirTests.cpp)

target_include_directories(SaturAirTests PRIVATE "${CMAKE_SOURCE_DIR}/Source")

# Os DSPs do projeto sao header-only: basta os modulos do JUCE que eles usam.
target_link_libraries(SaturAirTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_recommended_config_flags)

add_test(NAME SaturAirTests COMMAND SaturAirTests)
