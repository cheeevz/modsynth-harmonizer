#include "modsynth/CycleBuffer.hpp"

namespace modsynth {

void CycleBuffer::reset()
{
    history_.fill(0.0f);
    table_.fill(0.0f);
    writePos_ = 0;
    length_ = 0;
}

void CycleBuffer::write(float x)
{
    history_[writePos_] = x;
    writePos_ = (writePos_ + 1) % kMaxCycleLength;
}

void CycleBuffer::capture(std::size_t length)
{
    if (length == 0 || length >= kMaxCycleLength) {
        return;
    }
    // Le plus ancien des `length` derniers échantillons.
    std::size_t readPos = (writePos_ + kMaxCycleLength - length) % kMaxCycleLength;
    for (std::size_t i = 0; i < length; ++i) {
        table_[i] = history_[readPos];
        readPos = (readPos + 1) % kMaxCycleLength;
    }
    length_ = length;
}

} // namespace modsynth
