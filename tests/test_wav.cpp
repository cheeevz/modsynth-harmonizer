#include <doctest/doctest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "modsynth/host/Signals.hpp"
#include "modsynth/host/Wav.hpp"

using namespace modsynth::host;

TEST_CASE("WAV : écriture puis relecture en 24 bits")
{
    SignalSpec spec;
    spec.waveform = Waveform::Saw;
    spec.frequencyHz = 220.0f;
    spec.durationS = 0.1f;
    spec.sampleRate = 44100.0f;
    const auto original = generate(spec);

    const std::string path = "test_roundtrip.wav";
    std::string error;
    REQUIRE(writeWav(path, original, spec.sampleRate, error));

    WavData read;
    REQUIRE(readWav(path, read, error));
    std::remove(path.c_str());

    CHECK(read.sampleRate == doctest::Approx(44100.0f));
    CHECK(read.channels == 1u);
    REQUIRE(read.samples.size() == original.size());
    for (std::size_t i = 0; i < original.size(); ++i) {
        // Erreur de quantification en 24 bits : environ 1,2e-7.
        REQUIRE(read.samples[i] == doctest::Approx(original[i]).epsilon(1e-5));
    }
}

TEST_CASE("WAV : un fichier absent renvoie une erreur")
{
    WavData data;
    std::string error;
    CHECK_FALSE(readWav("fichier_inexistant.wav", data, error));
    CHECK_FALSE(error.empty());
}
