#include "modsynth/HarmonicVoice.hpp"

namespace modsynth {

float HarmonicVoice::process(const CycleBuffer& cycle)
{
    const std::size_t length = cycle.length();
    // Si la table a été raccourcie par une nouvelle capture, l'index est
    // ramené dans les bornes : c'est un saut brutal, donc un clic (limite 2019).
    index_ %= length;
    const float sample = cycle.at(index_);
    index_ = (index_ + harmonic_) % length;
    return sample;
}

} // namespace modsynth
