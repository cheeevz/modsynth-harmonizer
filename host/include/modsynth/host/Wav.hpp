#pragma once

#include <string>
#include <vector>

namespace modsynth::host {

/// Contenu audio lu dans un fichier WAV, ramené en mono.
struct WavData {
    float sampleRate = 0.0f;
    unsigned channels = 0;
    std::vector<float> samples; // valeurs entre -1 et 1
};

/// Lit un fichier WAV PCM (8, 16, 24 ou 32 bits) ou flottant 32 bits.
/// Seul le premier canal est conservé.
/// Retourne false et remplit `error` en cas d'échec.
bool readWav(const std::string& path, WavData& out, std::string& error);

/// Écrit un fichier WAV mono PCM 24 bits. Les valeurs sont écrêtées à ±1.
bool writeWav(const std::string& path, const std::vector<float>& samples,
              float sampleRate, std::string& error);

} // namespace modsynth::host
