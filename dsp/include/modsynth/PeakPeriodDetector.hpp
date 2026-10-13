#pragma once

#include <cstddef>

#include "modsynth/Config.hpp"

namespace modsynth {

/// Détecteur de période « méthode 2019 » : la période est la durée,
/// comptée en échantillons, entre deux maximums successifs du signal.
///
/// C'est une reproduction volontairement fidèle de la version d'école,
/// avec ses limites connues :
///  - la période est un nombre entier d'échantillons (comme un timer),
///    d'où une erreur de justesse qui augmente avec la fréquence ;
///  - le bruit déplace le maximum détecté, donc la période mesurée varie
///    d'un cycle à l'autre ;
///  - une forme d'onde avec plusieurs bosses par période peut déclencher
///    plusieurs détections.
///
/// Elle sert de point de comparaison « avant » pour la V2.
class PeakPeriodDetector {
public:
    void init(float sampleRate);
    void reset();

    /// Traite un échantillon. Retourne true quand un maximum (début de cycle)
    /// vient d'être détecté.
    bool process(float x);

    /// true si une période valide a été mesurée sur les derniers cycles.
    bool hasPeriod() const { return period_ != 0; }

    /// Période en échantillons, 0 si aucune période valide.
    std::size_t periodSamples() const { return period_; }

    /// Fréquence correspondante en Hz, 0 si aucune période valide.
    float frequencyHz() const;

private:
    // Un maximum n'est retenu que s'il dépasse ce pourcentage de l'enveloppe.
    static constexpr float kThresholdRatio = 0.8f;
    // En dessous de ce niveau, le signal est considéré comme du silence.
    static constexpr float kNoiseFloor = 0.01f;
    // Temps de décroissance de l'enveloppe, en secondes.
    static constexpr float kEnvelopeReleaseS = 0.05f;

    float sampleRate_ = kDefaultSampleRate;
    float envelope_ = 0.0f;
    float envelopeDecay_ = 0.0f;
    float prev_ = 0.0f;
    float prevPrev_ = 0.0f;
    std::size_t sinceLastPeak_ = 0;
    std::size_t period_ = 0;
    std::size_t minPeriod_ = 0;
    std::size_t maxPeriod_ = 0;
    bool hadPeak_ = false;
};

} // namespace modsynth
