#pragma once

#include <cstddef>

#include "modsynth/CycleBuffer.hpp"

namespace modsynth {

/// Voix « méthode 2019 » : relit le cycle capturé en sautant `harmonic`
/// échantillons à chaque pas. Lire un échantillon sur 3 fait parcourir
/// le cycle 3 fois plus vite, donc produit l'harmonique 3.
///
/// Limite connue : seuls les rapports entiers sont possibles
/// (harmoniques 2, 3, 4, 5...), donc pas d'accord mineur ni de septième.
/// La V2 remplacera ce pas entier par un accumulateur de phase fractionnaire
/// avec interpolation.
class HarmonicVoice {
public:
    void setHarmonic(std::size_t harmonic) { harmonic_ = harmonic; }
    std::size_t harmonic() const { return harmonic_; }

    void reset() { index_ = 0; }

    /// Retourne l'échantillon suivant de la voix. Le cycle doit être non vide.
    float process(const CycleBuffer& cycle);

private:
    std::size_t harmonic_ = 1;
    std::size_t index_ = 0;
};

} // namespace modsynth
