#include "modsynth/Harmonizer2019.hpp"

namespace modsynth {

void Harmonizer2019::init(float sampleRate)
{
    detector_.init(sampleRate);
    voices_[0].setHarmonic(3);
    voices_[1].setHarmonic(5);
    reset();
}

void Harmonizer2019::reset()
{
    detector_.reset();
    cycle_.reset();
    for (auto& voice : voices_) {
        voice.reset();
    }
}

void Harmonizer2019::setMix(float mix)
{
    if (mix < 0.0f) {
        mix = 0.0f;
    } else if (mix > 1.0f) {
        mix = 1.0f;
    }
    mix_ = mix;
}

void Harmonizer2019::process(const float* in, float* out, std::size_t count)
{
    for (std::size_t i = 0; i < count; ++i) {
        out[i] = processSample(in[i]);
    }
}

float Harmonizer2019::processSample(float x)
{
    cycle_.write(x);

    if (detector_.process(x) && detector_.hasPeriod()) {
        cycle_.capture(detector_.periodSamples());
    }

    float voices = 0.0f;
    // Sans période valide, les voix sont coupées net (pas de fondu en 2019).
    if (detector_.hasPeriod() && cycle_.length() > 0) {
        for (auto& voice : voices_) {
            voices += voice.process(cycle_);
        }
        voices *= 1.0f / static_cast<float>(voices_.size());
    }

    return (1.0f - mix_) * x + mix_ * voices;
}

} // namespace modsynth
