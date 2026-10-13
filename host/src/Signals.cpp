#include "modsynth/host/Signals.hpp"

#include <cmath>
#include <random>

namespace modsynth::host {

namespace {
constexpr double kTwoPi = 6.283185307179586;
}

bool parseWaveform(const std::string& name, Waveform& out)
{
    if (name == "sine") { out = Waveform::Sine; return true; }
    if (name == "saw") { out = Waveform::Saw; return true; }
    if (name == "square") { out = Waveform::Square; return true; }
    if (name == "triangle") { out = Waveform::Triangle; return true; }
    return false;
}

std::vector<float> generate(const SignalSpec& spec)
{
    const auto count = static_cast<std::size_t>(spec.durationS * spec.sampleRate);
    std::vector<float> out(count);

    std::mt19937 rng(spec.noiseSeed);
    std::uniform_real_distribution<float> noise(-1.0f, 1.0f);

    const double phaseStep = static_cast<double>(spec.frequencyHz) / spec.sampleRate;
    double phase = 0.0; // entre 0 et 1

    for (std::size_t i = 0; i < count; ++i) {
        double v = 0.0;
        switch (spec.waveform) {
        case Waveform::Sine:     v = std::sin(kTwoPi * phase); break;
        case Waveform::Saw:      v = 2.0 * phase - 1.0; break;
        case Waveform::Square:   v = (phase < 0.5) ? 1.0 : -1.0; break;
        case Waveform::Triangle: v = (phase < 0.5) ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase; break;
        }
        float sample = spec.amplitude * static_cast<float>(v);
        if (spec.noiseAmplitude > 0.0f) {
            sample += spec.noiseAmplitude * noise(rng);
        }
        out[i] = sample;

        phase += phaseStep;
        phase -= std::floor(phase);
    }
    return out;
}

std::vector<float> generateSequence(const std::vector<SignalSpec>& notes)
{
    std::vector<float> out;
    for (const auto& note : notes) {
        const auto part = generate(note);
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

} // namespace modsynth::host
