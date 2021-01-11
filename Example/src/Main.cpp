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
    Timer() : m_Start(std::chrono::high_resolution_clock::now()) {}

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

void PrintMidiEvents(std::unique_ptr<MidiParser>& parser) {
    std::cout << std::hex;
    for (MidiTrack& track : *parser) {
        std::cout << "----------------------- New track -----------------------\n";
        for (int i = 0; i < track.GetEventCount(); i++) {
            if (track[i]->GetType() == MidiEventType::NoteOn) {
                std::cout << MidiUtilities::NoteToString((MidiEvent*)track[i]) << "\n";
            } else if (track[i]->GetCategory() == EventCategory::Meta) {
                MetaEvent& metaEvent = *reinterpret_cast<MetaEvent*>(track[i]);
                std::cout << "Meta event: ";
                for (int j = 0; j < metaEvent.GetSize(); j++)
                    std::cout << std::hex << (int)metaEvent[j] << " ";
                std::cout << "\n";
            }
        }
    }
    std::cout << std::dec;
}

int main() {
    std::unique_ptr<MidiParser> reader = std::make_unique<MidiParser>();
    for (int i = 0; i < 100; i++) {
        Timer timer;
        reader->Open("../../Example/assets/Type1/SpanishFlea.mid");
    }

    PrintMidiEvents(reader);

    auto [minutes, seconds] = reader->GetDurationMinutesAndSeconds();
    std::cout << "MIDI duration: " << minutes << " minutes and " << seconds << " seconds\n";

    std::cout << s_AllocCount << " allocations\n";
}
