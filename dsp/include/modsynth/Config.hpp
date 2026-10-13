#pragma once

#include <cstddef>

namespace modsynth {

/// Fréquence d'échantillonnage de référence (EP-01).
inline constexpr float kDefaultSampleRate = 48000.0f;

/// Taille d'un bloc audio : 48 échantillons, soit 1 ms à 48 kHz.
inline constexpr std::size_t kBlockSize = 48;

/// Plage de fondamentales gérées (EP-03).
inline constexpr float kMinFrequencyHz = 80.0f;
inline constexpr float kMaxFrequencyHz = 1000.0f;

/// Longueur maximale d'un cycle capturé, en échantillons.
/// Une période à 80 Hz dure 600 échantillons à 48 kHz et 1200 à 96 kHz.
inline constexpr std::size_t kMaxCycleLength = 2048;

} // namespace modsynth
