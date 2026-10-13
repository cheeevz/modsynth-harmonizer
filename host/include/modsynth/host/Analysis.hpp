#pragma once

#include <cstddef>
#include <vector>

namespace modsynth::host {

/// Amplitude crête de la composante à `frequencyHz` dans la portion
/// [start, start + length) du signal (algorithme de Goertzel, fenêtre de Hann).
/// Pour un sinus d'amplitude A exactement à cette fréquence, renvoie environ A.
float toneAmplitude(const std::vector<float>& signal, std::size_t start, std::size_t length,
                    float frequencyHz, float sampleRate);

/// Écart en cents entre une fréquence mesurée et une fréquence de référence.
/// 100 cents = 1 demi-ton.
float centsBetween(float measuredHz, float referenceHz);

/// Statistiques simples sur une série de valeurs.
struct Stats {
    float min = 0.0f;
    float max = 0.0f;
    float mean = 0.0f;
    std::size_t count = 0;
};

Stats computeStats(const std::vector<float>& values);

} // namespace modsynth::host
