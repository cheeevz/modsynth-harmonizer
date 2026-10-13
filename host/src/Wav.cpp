#include "modsynth/host/Wav.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>

namespace modsynth::host {

namespace {

constexpr std::uint16_t kFormatPcm = 1;
constexpr std::uint16_t kFormatFloat = 3;
constexpr std::uint16_t kFormatExtensible = 0xFFFE;

std::uint16_t readU16(const unsigned char* p)
{
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

std::uint32_t readU32(const unsigned char* p)
{
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

void writeU16(std::ofstream& f, std::uint16_t v)
{
    const unsigned char b[2] = {static_cast<unsigned char>(v & 0xFF),
                                static_cast<unsigned char>((v >> 8) & 0xFF)};
    f.write(reinterpret_cast<const char*>(b), 2);
}

void writeU32(std::ofstream& f, std::uint32_t v)
{
    const unsigned char b[4] = {static_cast<unsigned char>(v & 0xFF),
                                static_cast<unsigned char>((v >> 8) & 0xFF),
                                static_cast<unsigned char>((v >> 16) & 0xFF),
                                static_cast<unsigned char>((v >> 24) & 0xFF)};
    f.write(reinterpret_cast<const char*>(b), 4);
}

float decodeSample(const unsigned char* p, std::uint16_t format, std::uint16_t bits)
{
    if (format == kFormatFloat && bits == 32) {
        float v;
        std::memcpy(&v, p, sizeof v);
        return v;
    }
    switch (bits) {
    case 8:
        return (static_cast<float>(p[0]) - 128.0f) / 128.0f;
    case 16:
        return static_cast<float>(static_cast<std::int16_t>(readU16(p))) / 32768.0f;
    case 24: {
        std::int32_t v = static_cast<std::int32_t>(p[0] | (p[1] << 8) | (p[2] << 16));
        if (v & 0x800000) {
            v |= ~0xFFFFFF; // extension de signe
        }
        return static_cast<float>(v) / 8388608.0f;
    }
    case 32:
        return static_cast<float>(static_cast<double>(static_cast<std::int32_t>(readU32(p))) /
                                  2147483648.0);
    default:
        return 0.0f;
    }
}

} // namespace

bool readWav(const std::string& path, WavData& out, std::string& error)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "impossible d'ouvrir " + path;
        return false;
    }
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());

    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        error = "ce n'est pas un fichier WAV (RIFF/WAVE)";
        return false;
    }

    std::uint16_t format = 0;
    std::uint16_t channels = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bits = 0;
    const unsigned char* data = nullptr;
    std::size_t dataSize = 0;

    std::size_t pos = 12;
    while (pos + 8 <= bytes.size()) {
        const unsigned char* chunk = bytes.data() + pos;
        const std::uint32_t chunkSize = readU32(chunk + 4);
        const std::size_t available = std::min<std::size_t>(chunkSize, bytes.size() - pos - 8);

        if (std::memcmp(chunk, "fmt ", 4) == 0 && available >= 16) {
            format = readU16(chunk + 8);
            channels = readU16(chunk + 10);
            sampleRate = readU32(chunk + 12);
            bits = readU16(chunk + 22);
            if (format == kFormatExtensible && available >= 26) {
                format = readU16(chunk + 32); // deux premiers octets du sous-format
            }
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            data = chunk + 8;
            dataSize = available;
        }
        pos += 8 + chunkSize + (chunkSize & 1u); // les blocs sont alignés sur 2 octets
    }

    if (format != kFormatPcm && format != kFormatFloat) {
        error = "format WAV non pris en charge (seuls PCM et flottant 32 bits)";
        return false;
    }
    if (channels == 0 || bits == 0 || sampleRate == 0 || data == nullptr) {
        error = "en-tête WAV incomplet";
        return false;
    }
    if (bits != 8 && bits != 16 && bits != 24 && bits != 32) {
        error = "résolution non prise en charge : " + std::to_string(bits) + " bits";
        return false;
    }

    const std::size_t bytesPerSample = bits / 8u;
    const std::size_t frameSize = bytesPerSample * channels;
    const std::size_t frames = dataSize / frameSize;

    out.sampleRate = static_cast<float>(sampleRate);
    out.channels = channels;
    out.samples.resize(frames);
    for (std::size_t i = 0; i < frames; ++i) {
        out.samples[i] = decodeSample(data + i * frameSize, format, bits);
    }
    return true;
}

bool writeWav(const std::string& path, const std::vector<float>& samples,
              float sampleRate, std::string& error)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        error = "impossible de créer " + path;
        return false;
    }

    const std::uint16_t channels = 1;
    const std::uint16_t bits = 24;
    const std::uint32_t rate = static_cast<std::uint32_t>(std::lround(sampleRate));
    const std::uint16_t blockAlign = channels * bits / 8;
    const std::uint32_t dataSize = static_cast<std::uint32_t>(samples.size() * blockAlign);

    file.write("RIFF", 4);
    writeU32(file, 36 + dataSize);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    writeU32(file, 16);
    writeU16(file, kFormatPcm);
    writeU16(file, channels);
    writeU32(file, rate);
    writeU32(file, rate * blockAlign);
    writeU16(file, blockAlign);
    writeU16(file, bits);
    file.write("data", 4);
    writeU32(file, dataSize);

    for (float s : samples) {
        const float clipped = std::clamp(s, -1.0f, 1.0f);
        const std::int32_t v = static_cast<std::int32_t>(std::lround(clipped * 8388607.0f));
        const unsigned char b[3] = {static_cast<unsigned char>(v & 0xFF),
                                    static_cast<unsigned char>((v >> 8) & 0xFF),
                                    static_cast<unsigned char>((v >> 16) & 0xFF)};
        file.write(reinterpret_cast<const char*>(b), 3);
    }

    if (!file) {
        error = "erreur d'écriture dans " + path;
        return false;
    }
    return true;
}

} // namespace modsynth::host
