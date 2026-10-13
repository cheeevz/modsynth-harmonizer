#pragma once

#include <array>
#include <cstddef>

#include "modsynth/Config.hpp"

namespace modsynth {

/// Mémorise les derniers échantillons reçus et capture un cycle complet
/// du signal dans une table, relue ensuite par les voix.
///
/// Version 2019 : un seul tableau, écrasé à chaque nouvelle capture pendant
/// qu'il est relu. C'est l'une des causes des clics de la version d'origine ;
/// la V2 la remplacera par un double tampon.
///
/// Aucune allocation dynamique : tout est dimensionné à la compilation.
class CycleBuffer {
public:
    void reset();

    /// Ajoute un échantillon à l'historique circulaire.
    void write(float x);

    /// Copie les `length` derniers échantillons de l'historique dans la table.
    void capture(std::size_t length);

    /// Longueur du cycle capturé, 0 si rien n'a encore été capturé.
    std::size_t length() const { return length_; }

    /// Échantillon `index` du cycle capturé (l'index boucle sur la longueur).
    /// Ne doit être appelé que si length() > 0.
    float at(std::size_t index) const { return table_[index % length_]; }

private:
    std::array<float, kMaxCycleLength> history_{};
    std::array<float, kMaxCycleLength> table_{};
    std::size_t writePos_ = 0;
    std::size_t length_ = 0;
};

} // namespace modsynth
