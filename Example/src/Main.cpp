#include <MidiParser.h>
#include <MidiUtilities/MidiUtilities.h>

#include <iostream>
#include <chrono>
#include <memory>

static uint32_t s_AllocCount;
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

        auto startMicroseconds = std::chrono::time_point_cast<std::chrono::microseconds>(m_Start).time_since_epoch().count();
        auto stopMicroseconds = std::chrono::time_point_cast<std::chrono::microseconds>(m_Stop).time_since_epoch().count();
        int64_t microseconds = stopMicroseconds - startMicroseconds;
        std::cout << "Time: " << microseconds << " microseconds!\n";
    }
};

void ErrorCallback(const std::string& msg) {
    std::cout << "Error: " << msg << "\n";
}

int main() {
    std::unique_ptr<MidiParser> reader = std::make_unique<MidiParser>(ErrorCallback);
    {
        Timer timer;
        reader->Open("../../Example/assets/Type1/SpanishFlea.mid");
    }

#if 0
    std::cout << std::hex;
    for (int track = 0; track < reader->GetTrackCount(); track++) {
        std::cout << "Track " << track << ":\n";
        for (int event = 0; event < (*reader)[track].GetEventCount(); event++) {
            Event* thing = (*reader)[track][event];
            if (thing->Type == MidiEventType::Meta) {
                MetaEvent* metaEvent = static_cast<MetaEvent*>(thing);
            } else {
                MidiEvent* midiEvent = static_cast<MidiEvent*>(thing);
                std::cout << "Note: " << MidiUtilities::ConvertNote(*midiEvent) << "\n";
            }
        }
    }
    std::cout << std::dec;
#endif

    //auto [minutes, seconds] = reader->GetDuration();
    //std::cout << "MIDI duration: " << minutes << " minutes and " << seconds << " seconds\n";

    std::cout << s_AllocCount << " heap allocations\n";
}
