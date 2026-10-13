#pragma once

#include <array>
#include <cstddef>

#include "modsynth/CycleBuffer.hpp"
#include "modsynth/HarmonicVoice.hpp"
#include "modsynth/PeakPeriodDetector.hpp"

namespace modsynth {

/// V1 — reproduction de l'harmoniseur ModSynth de 2019.
///
/// Chaîne de traitement, par échantillon :
///  1. le signal est mémorisé dans l'historique du CycleBuffer ;
///  2. le PeakPeriodDetector mesure la période entre deux maximums ;
///  3. à chaque maximum, un cycle complet est copié dans la table ;
///  4. deux voix relisent cette table pour produire les harmoniques 3 et 5 ;
///  5. les voix sont mélangées au signal d'origine.
///
/// Mode « Harmoniques 2019 » de la spécification (EF-03).
class Harmonizer2019 {
public:
    void init(float sampleRate);
    void reset();

    /// Mélange entre signal d'origine (0) et voix générées (1).
    void setMix(float mix);

    /// Traite un bloc d'échantillons. `in` et `out` peuvent être identiques.
    void process(const float* in, float* out, std::size_t count);

    const PeakPeriodDetector& detector() const { return detector_; }

private:
    float processSample(float x);

    PeakPeriodDetector detector_;
    CycleBuffer cycle_;
    std::array<HarmonicVoice, 2> voices_;
    float mix_ = 0.5f;
};

} // namespace modsynth
