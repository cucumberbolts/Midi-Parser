#include <MidiParser/MidiParser.h>

#include <iostream>
#include <chrono>

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
    MidiParser reader(ErrorCallback);
    for (int i = 0; i < 30; i++) {
        Timer timer;
        reader.Open("../../Example/assets/SpanishFlea.mid");
    }

    std::cout << s_AllocCount << " heap allocations\n";
}
