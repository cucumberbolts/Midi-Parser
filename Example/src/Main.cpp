#include <MidiParser.h>
#include <MidiUtilities/MidiUtilities.h>

#include <iostream>
#include <chrono>
#include <memory>

static uint32_t s_AllocCount = 0;
void* operator new(size_t size) {
    s_AllocCount++;
    return malloc(size);
}

class Timer {
private:
    std::chrono::high_resolution_clock::time_point m_Start;
    std::chrono::high_resolution_clock::time_point m_Stop;
public:
    Timer() {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    ~Timer() {
        Stop();
    }

    void Stop() {
        m_Stop = std::chrono::high_resolution_clock::now();

        int64_t startMicroseconds = std::chrono::time_point_cast<std::chrono::microseconds>(m_Start).time_since_epoch().count();
        int64_t stopMicroseconds = std::chrono::time_point_cast<std::chrono::microseconds>(m_Stop).time_since_epoch().count();
        int64_t microseconds = stopMicroseconds - startMicroseconds;
        std::cout << "Time: " << microseconds << " microseconds!\n";
    }
};

int main() {
    std::unique_ptr<MidiParser> reader = std::make_unique<MidiParser>();
    {
        Timer timer;
        reader->Open("../../Example/assets/Type1/SpanishFlea.mid");
    }
#if 0
    std::cout << std::hex;
    for (MidiTrack& track : *reader) {
        std::cout << "----------------------- New track -----------------------\n";
        for (MidiEvent& event : track) {
            std::string noteName = MidiUtilities::ConvertNote(event);
            if (!noteName.empty())
                std::cout << noteName << "\n";
        }
    }
#endif
    auto [minutes, seconds] = reader->GetDurationMinutesAndSeconds();
    std::cout << std::hex << "MIDI duration: " << minutes << " minutes and " << seconds << " seconds\n";

    std::cout << s_AllocCount << " heap allocations\n";
}
