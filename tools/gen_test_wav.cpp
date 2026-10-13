// Génère un fichier WAV de test : un signal périodique, éventuellement bruité.
//
// Usage :
//   gen_test_wav <sortie.wav> [fréquence_Hz] [forme] [durée_s] [bruit]
//
//   forme : sine | saw | square | triangle   (défaut : saw)
//   bruit : amplitude du bruit blanc ajouté, par exemple 0.02 (défaut : 0)
//
// Exemple :
//   gen_test_wav saw_220.wav 220 saw 3 0.02

#include <cstdio>
#include <cstdlib>
#include <string>

#include "modsynth/host/Signals.hpp"
#include "modsynth/host/Wav.hpp"

int main(int argc, char** argv)
{
    using namespace modsynth::host;

    if (argc < 2) {
        std::fprintf(stderr,
                     "Usage : %s <sortie.wav> [fréquence_Hz] [sine|saw|square|triangle] "
                     "[durée_s] [bruit]\n",
                     argv[0]);
        return 1;
    }

    SignalSpec spec;
    spec.waveform = Waveform::Saw;
    spec.durationS = 3.0f;
    if (argc > 2) spec.frequencyHz = std::strtof(argv[2], nullptr);
    if (argc > 3 && !parseWaveform(argv[3], spec.waveform)) {
        std::fprintf(stderr, "Forme d'onde inconnue : %s\n", argv[3]);
        return 1;
    }
    if (argc > 4) spec.durationS = std::strtof(argv[4], nullptr);
    if (argc > 5) spec.noiseAmplitude = std::strtof(argv[5], nullptr);

    std::string error;
    if (!writeWav(argv[1], generate(spec), spec.sampleRate, error)) {
        std::fprintf(stderr, "Erreur : %s\n", error.c_str());
        return 1;
    }
    std::printf("Écrit %s : %s, %.1f Hz, %.1f s, bruit %.3f\n", argv[1],
                argc > 3 ? argv[3] : "saw", static_cast<double>(spec.frequencyHz),
                static_cast<double>(spec.durationS), static_cast<double>(spec.noiseAmplitude));
    return 0;
}
