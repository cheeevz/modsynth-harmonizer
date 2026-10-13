#include <doctest/doctest.h>

#include <cmath>
#include <vector>

#include "modsynth/PeakPeriodDetector.hpp"
#include "modsynth/host/Analysis.hpp"
#include "modsynth/host/Signals.hpp"

using namespace modsynth;
using namespace modsynth::host;

namespace {

constexpr float kFs = 48000.0f;

/// Fait passer le signal dans le détecteur et relève la fréquence mesurée
/// à chaque maximum, après une seconde de stabilisation (`skipS`).
std::vector<float> measuredFrequencies(const std::vector<float>& signal, float skipS = 0.1f)
{
    PeakPeriodDetector detector;
    detector.init(kFs);
    std::vector<float> freqs;
    const auto skip = static_cast<std::size_t>(skipS * kFs);
    for (std::size_t i = 0; i < signal.size(); ++i) {
        if (detector.process(signal[i]) && detector.hasPeriod() && i >= skip) {
            freqs.push_back(detector.frequencyHz());
        }
    }
    return freqs;
}

SignalSpec note(Waveform w, float hz, float noise = 0.0f)
{
    SignalSpec s;
    s.waveform = w;
    s.frequencyHz = hz;
    s.noiseAmplitude = noise;
    s.durationS = 1.0f;
    s.sampleRate = kFs;
    return s;
}

} // namespace

TEST_CASE("Détection de période 2019 : signaux propres dans la plage")
{
    // Erreur tolérée : celle de la période entière (« timer »), qui grandit
    // avec la fréquence. À 1 kHz, une période dure 48 échantillons :
    // une erreur d'un échantillon vaut environ 36 cents.
    struct Case { Waveform w; float hz; float toleranceCents; };
    const Case cases[] = {
        {Waveform::Sine, 110.0f, 10.0f},
        {Waveform::Sine, 220.0f, 10.0f},
        {Waveform::Sine, 440.0f, 20.0f},
        {Waveform::Saw, 110.0f, 10.0f},
        {Waveform::Saw, 220.0f, 10.0f},
        {Waveform::Square, 220.0f, 10.0f},
        {Waveform::Triangle, 330.0f, 15.0f},
    };

    for (const auto& c : cases) {
        CAPTURE(c.hz);
        const auto freqs = measuredFrequencies(generate(note(c.w, c.hz)));
        REQUIRE_FALSE(freqs.empty());
        const Stats s = computeStats(freqs);
        CHECK(std::fabs(centsBetween(s.mean, c.hz)) < c.toleranceCents);
    }
}

TEST_CASE("Détection de période 2019 : pas de période sur le silence")
{
    const std::vector<float> silence(static_cast<std::size_t>(kFs), 0.0f);
    PeakPeriodDetector detector;
    detector.init(kFs);
    for (float x : silence) {
        detector.process(x);
    }
    CHECK_FALSE(detector.hasPeriod());
    CHECK(detector.frequencyHz() == 0.0f);
}

TEST_CASE("Détection de période 2019 : fréquence trop grave ignorée")
{
    // 40 Hz est sous la plage gérée (80 Hz à 1 kHz, EP-03).
    const auto freqs = measuredFrequencies(generate(note(Waveform::Sine, 40.0f)));
    CHECK(freqs.empty());
}

TEST_CASE("Détection de période 2019 : suit un changement de note")
{
    const auto signal = generateSequence({note(Waveform::Saw, 110.0f), note(Waveform::Saw, 220.0f)});
    PeakPeriodDetector detector;
    detector.init(kFs);
    for (float x : signal) {
        detector.process(x);
    }
    REQUIRE(detector.hasPeriod());
    CHECK(std::fabs(centsBetween(detector.frequencyHz(), 220.0f)) < 10.0f);
}

TEST_CASE("Limite connue 2019 : le bruit rend la hauteur instable")
{
    // Ce test ne vérifie pas une exigence : il documente le défaut de la
    // méthode d'origine, que la V2 doit corriger (EP-07 : ≤ 3 cents).
    const auto clean = computeStats(measuredFrequencies(generate(note(Waveform::Sine, 220.0f))));
    const auto noisy =
        computeStats(measuredFrequencies(generate(note(Waveform::Sine, 220.0f, 0.05f))));

    const float cleanJitter = centsBetween(clean.max, clean.min);
    const float noisyJitter = centsBetween(noisy.max, noisy.min);
    MESSAGE("Instabilité sinus 220 Hz propre : " << cleanJitter << " cents");
    MESSAGE("Instabilité sinus 220 Hz + 5 % de bruit : " << noisyJitter << " cents");

    CHECK(noisyJitter > cleanJitter);
}
