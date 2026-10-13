#include <doctest/doctest.h>

#include <cmath>
#include <vector>

#include "modsynth/Config.hpp"
#include "modsynth/Harmonizer2019.hpp"
#include "modsynth/host/Analysis.hpp"
#include "modsynth/host/Signals.hpp"

using namespace modsynth;
using namespace modsynth::host;

namespace {

constexpr float kFs = 48000.0f;

std::vector<float> runHarmonizer(const std::vector<float>& input, float mix)
{
    Harmonizer2019 harmonizer;
    harmonizer.init(kFs);
    harmonizer.setMix(mix);
    std::vector<float> output(input.size());
    for (std::size_t pos = 0; pos < input.size(); pos += kBlockSize) {
        const std::size_t count = std::min(kBlockSize, input.size() - pos);
        harmonizer.process(input.data() + pos, output.data() + pos, count);
    }
    return output;
}

std::vector<float> sine(float hz, float seconds = 1.0f)
{
    SignalSpec s;
    s.frequencyHz = hz;
    s.durationS = seconds;
    s.sampleRate = kFs;
    return generate(s);
}

} // namespace

TEST_CASE("Harmoniseur 2019 : produit les harmoniques 3 et 5")
{
    const float f0 = 220.0f;
    const auto out = runHarmonizer(sine(f0), 1.0f); // voix seules

    // Analyse sur 0,5 s, après la phase d'accrochage.
    const auto start = static_cast<std::size_t>(0.25f * kFs);
    const auto length = static_cast<std::size_t>(0.5f * kFs);

    // Fréquences réellement produites : la période est un nombre entier
    // d'échantillons (218 au lieu de 218,18), d'où un léger décalage.
    const float period = std::round(kFs / f0);
    const float h3 = 3.0f * kFs / period;
    const float h5 = 5.0f * kFs / period;

    const float a3 = toneAmplitude(out, start, length, h3, kFs);
    const float a5 = toneAmplitude(out, start, length, h5, kFs);
    const float a1 = toneAmplitude(out, start, length, f0, kFs);
    MESSAGE("Amplitudes : fondamentale " << a1 << ", harmonique 3 " << a3 << ", harmonique 5 " << a5);

    CHECK(a3 > 0.3f);
    CHECK(a5 > 0.3f);
    CHECK(a1 < 0.05f); // la fondamentale n'est pas dans les voix
    CHECK(std::fabs(centsBetween(h3, 3.0f * f0)) < 5.0f);
    CHECK(std::fabs(centsBetween(h5, 5.0f * f0)) < 5.0f);
}

TEST_CASE("Harmoniseur 2019 : mix à 0, le signal passe sans modification")
{
    const auto in = sine(330.0f, 0.5f);
    const auto out = runHarmonizer(in, 0.0f);
    REQUIRE(out.size() == in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        REQUIRE(out[i] == doctest::Approx(in[i]));
    }
}

TEST_CASE("Harmoniseur 2019 : silence en entrée, silence en sortie")
{
    const std::vector<float> silence(static_cast<std::size_t>(kFs / 2.0f), 0.0f);
    const auto out = runHarmonizer(silence, 1.0f);
    for (float x : out) {
        REQUIRE(x == 0.0f);
    }
}

TEST_CASE("Harmoniseur 2019 : le mix est borné entre 0 et 1")
{
    const auto in = sine(220.0f, 0.2f);
    // Un mix hors bornes doit se comporter comme la borne la plus proche.
    CHECK(runHarmonizer(in, -1.0f) == runHarmonizer(in, 0.0f));
    CHECK(runHarmonizer(in, 2.0f) == runHarmonizer(in, 1.0f));
}
