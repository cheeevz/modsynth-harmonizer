cmake_minimum_required(VERSION 3.16)
project(modsynth_harmonizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# Cœur DSP : indépendant du matériel, compilé pour le PC et pour le STM32.
# Les mêmes contraintes que sur la cible sont appliquées ici :
# pas d'exceptions, pas de RTTI, avertissement sur toute promotion en double.
# ---------------------------------------------------------------------------
add_library(modsynth_dsp
  dsp/src/PeakPeriodDetector.cpp
  dsp/src/CycleBuffer.cpp
  dsp/src/HarmonicVoice.cpp
  dsp/src/Harmonizer2019.cpp
)
target_include_directories(modsynth_dsp PUBLIC dsp/include)

if(MSVC)
  target_compile_options(modsynth_dsp PRIVATE /W4)
else()
  target_compile_options(modsynth_dsp PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion
    -fno-exceptions -fno-rtti)
endif()

# ---------------------------------------------------------------------------
# Outils et tests sur PC (désactivés pour la compilation embarquée)
# ---------------------------------------------------------------------------
option(MODSYNTH_BUILD_HOST "Construire les outils et les tests pour PC" ON)

if(MODSYNTH_BUILD_HOST)
  add_library(modsynth_host
    host/src/Wav.cpp
    host/src/Signals.cpp
    host/src/Analysis.cpp
  )
  target_include_directories(modsynth_host PUBLIC host/include)

  add_executable(gen_test_wav tools/gen_test_wav.cpp)
  target_link_libraries(gen_test_wav PRIVATE modsynth_host)

  add_executable(harmonize_wav tools/harmonize_wav.cpp)
  target_link_libraries(harmonize_wav PRIVATE modsynth_dsp modsynth_host)

  include(FetchContent)
  FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(doctest)

  enable_testing()
  add_executable(dsp_tests
    tests/main.cpp
    tests/test_period_detector.cpp
    tests/test_harmonizer2019.cpp
    tests/test_wav.cpp
  )
  target_link_libraries(dsp_tests PRIVATE modsynth_dsp modsynth_host doctest::doctest)
  add_test(NAME dsp_tests COMMAND dsp_tests)
endif()
