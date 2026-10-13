#include "modsynth/host/Analysis.hpp"

#include <algorithm>
#include <cmath>

namespace modsynth::host {

namespace {
constexpr double kTwoPi = 6.283185307179586;
}

float toneAmplitude(const std::vector<float>& signal, std::size_t start, std::size_t length,
                    float frequencyHz, float sampleRate)
{
    if (start >= signal.size() || length < 2) {
        return 0.0f;
    }
    length = std::min(length, signal.size() - start);

    const double omega = kTwoPi * frequencyHz / sampleRate;
    const double coeff = 2.0 * std::cos(omega);
    double s1 = 0.0;
    double s2 = 0.0;
    double windowSum = 0.0;

    for (std::size_t i = 0; i < length; ++i) {
        const double w = 0.5 - 0.5 * std::cos(kTwoPi * static_cast<double>(i) /
                                              static_cast<double>(length - 1));
        windowSum += w;
        const double s0 = signal[start + i] * w + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    return static_cast<float>(2.0 * std::sqrt(std::max(power, 0.0)) / windowSum);
}

float centsBetween(float measuredHz, float referenceHz)
{
    return static_cast<float>(1200.0 * std::log2(static_cast<double>(measuredHz) / referenceHz));
}

Stats computeStats(const std::vector<float>& values)
{
    Stats s;
    if (values.empty()) {
        return s;
    }
    s.min = *std::min_element(values.begin(), values.end());
    s.max = *std::max_element(values.begin(), values.end());
    double sum = 0.0;
    for (float v : values) {
        sum += v;
    }
    s.mean = static_cast<float>(sum / static_cast<double>(values.size()));
    s.count = values.size();
    return s;
}

} // namespace modsynth::host
