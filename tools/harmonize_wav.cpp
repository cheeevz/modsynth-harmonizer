// Fait passer un fichier WAV dans l'harmoniseur, comme le ferait le module,
// et affiche des mesures sur la détection de hauteur.
//
// Usage :
//   harmonize_wav <entrée.wav> <sortie.wav> [mix]
//
//   mix : 0 = signal d'origine seul, 1 = voix générées seules (défaut : 0.5)
//
// Le traitement se fait par blocs de 48 échantillons, comme sur la carte.
// Pour un enregistrement stéréo, seul le canal gauche est traité.

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "modsynth/Config.hpp"
#include "modsynth/Harmonizer2019.hpp"
#include "modsynth/host/Analysis.hpp"
#include "modsynth/host/Wav.hpp"

int main(int argc, char** argv)
{
    using namespace modsynth;
    using namespace modsynth::host;

    if (argc < 3) {
        std::fprintf(stderr, "Usage : %s <entrée.wav> <sortie.wav> [mix 0..1]\n", argv[0]);
        return 1;
    }
    const float mix = (argc > 3) ? std::strtof(argv[3], nullptr) : 0.5f;

    WavData input;
    std::string error;
    if (!readWav(argv[1], input, error)) {
        std::fprintf(stderr, "Erreur : %s\n", error.c_str());
        return 1;
    }

    Harmonizer2019 harmonizer;
    harmonizer.init(input.sampleRate);
    harmonizer.setMix(mix);

    std::vector<float> output(input.samples.size());
    std::vector<float> detectedHz; // une mesure par bloc où une période est connue

    for (std::size_t pos = 0; pos < input.samples.size(); pos += kBlockSize) {
        const std::size_t count = std::min(kBlockSize, input.samples.size() - pos);
        harmonizer.process(input.samples.data() + pos, output.data() + pos, count);
        if (harmonizer.detector().hasPeriod()) {
            detectedHz.push_back(harmonizer.detector().frequencyHz());
        }
    }

    if (!writeWav(argv[2], output, input.sampleRate, error)) {
        std::fprintf(stderr, "Erreur : %s\n", error.c_str());
        return 1;
    }

    const std::size_t blocks = (input.samples.size() + kBlockSize - 1) / kBlockSize;
    std::printf("Entrée  : %s (%.0f Hz, %u canal(aux), %.2f s)\n", argv[1],
                static_cast<double>(input.sampleRate), input.channels,
                static_cast<double>(input.samples.size()) / static_cast<double>(input.sampleRate));
    std::printf("Sortie  : %s (mix %.2f)\n", argv[2], static_cast<double>(mix));
    std::printf("Période détectée sur %zu blocs sur %zu (%.0f %%)\n", detectedHz.size(), blocks,
                blocks ? 100.0 * static_cast<double>(detectedHz.size()) / static_cast<double>(blocks) : 0.0);

    if (!detectedHz.empty()) {
        const Stats s = computeStats(detectedHz);
        std::printf("Fréquence détectée : moyenne %.2f Hz, min %.2f Hz, max %.2f Hz\n",
                    static_cast<double>(s.mean), static_cast<double>(s.min),
                    static_cast<double>(s.max));
        std::printf("Instabilité (max - min) : %.1f cents\n",
                    static_cast<double>(centsBetween(s.max, s.min)));
        std::printf("(Valable pour une note tenue ; sur une mélodie, l'écart inclut "
                    "les changements de note.)\n");
    }
    return 0;
}
