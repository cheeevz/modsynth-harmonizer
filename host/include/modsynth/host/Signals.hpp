#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace modsynth::host {

enum class Waveform { Sine, Saw, Square, Triangle };

/// Convertit "sine", "saw", "square" ou "triangle". Retourne false si inconnu.
bool parseWaveform(const std::string& name, Waveform& out);

/// Paramètres d'un signal de test.
struct SignalSpec {
    Waveform waveform = Waveform::Sine;
    float frequencyHz = 220.0f;
    float amplitude = 0.8f;
    float noiseAmplitude = 0.0f;  // bruit blanc ajouté, 0 = aucun
    float durationS = 1.0f;
    float sampleRate = 48000.0f;
    std::uint32_t noiseSeed = 1;  // graine fixe : signaux reproductibles
};

/// Génère un signal périodique de test, éventuellement bruité.
/// Les formes d'onde sont « naïves » (non anti-repliées) : elles imitent
/// un VCO analogique numérisé, ce qui suffit pour tester la détection.
std::vector<float> generate(const SignalSpec& spec);

/// Concatène plusieurs notes, pour tester les changements de note.
std::vector<float> generateSequence(const std::vector<SignalSpec>& notes);

} // namespace modsynth::host
