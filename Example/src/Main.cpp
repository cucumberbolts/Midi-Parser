#include <MidiParser/MidiParser.h>

#include <iostream>

int main() {
    MidiParser reader("../../Example/assets/maRIO_TheME.mid");

    std::cout << std::hex;
    for (auto track : reader)
        for (auto event : track)
            std::cout << "Midi event: (" << (int)event.Type << ") " << (int)event.DataA << " " << (int)event.DataB << "\n";
}
