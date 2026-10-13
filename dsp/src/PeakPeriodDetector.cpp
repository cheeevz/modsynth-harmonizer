#include "modsynth/PeakPeriodDetector.hpp"

#include <cmath>

namespace modsynth {

void PeakPeriodDetector::init(float sampleRate)
{
    sampleRate_ = sampleRate;
    envelopeDecay_ = std::exp(-1.0f / (kEnvelopeReleaseS * sampleRate_));

    minPeriod_ = static_cast<std::size_t>(sampleRate_ / kMaxFrequencyHz);
    maxPeriod_ = static_cast<std::size_t>(sampleRate_ / kMinFrequencyHz);
    if (maxPeriod_ >= kMaxCycleLength) {
        maxPeriod_ = kMaxCycleLength - 1;
    }
    reset();
}

void PeakPeriodDetector::reset()
{
    envelope_ = 0.0f;
    prev_ = 0.0f;
    prevPrev_ = 0.0f;
    sinceLastPeak_ = 0;
    period_ = 0;
    hadPeak_ = false;
}

bool PeakPeriodDetector::process(float x)
{
    // Suivi d'enveloppe : montée instantanée, descente exponentielle.
    const float magnitude = std::fabs(x);
    envelope_ = (magnitude > envelope_) ? magnitude : envelope_ * envelopeDecay_;

    ++sinceLastPeak_;
    bool peakDetected = false;

    // L'échantillon précédent est un maximum local suffisamment haut.
    const bool isLocalMax = (prev_ > prevPrev_) && (prev_ >= x);
    const bool isHighEnough = (prev_ > kThresholdRatio * envelope_) && (prev_ > kNoiseFloor);

    if (isLocalMax && isHighEnough && sinceLastPeak_ > minPeriod_) {
        // Les deux maximums sont détectés avec le même retard d'un échantillon :
        // l'écart entre eux est donc directement le compteur.
        if (hadPeak_ && sinceLastPeak_ <= maxPeriod_) {
            period_ = sinceLastPeak_;
        }
        hadPeak_ = true;
        sinceLastPeak_ = 0;
        peakDetected = true;
    }

    // Plus de maximum depuis trop longtemps : signal perdu ou trop grave.
    if (sinceLastPeak_ > maxPeriod_) {
        period_ = 0;
        hadPeak_ = false;
    }

    prevPrev_ = prev_;
    prev_ = x;
    return peakDetected;
}

float PeakPeriodDetector::frequencyHz() const
{
    return (period_ != 0) ? sampleRate_ / static_cast<float>(period_) : 0.0f;
}

} // namespace modsynth
