#include <MidiParser/MidiParser.h>

#include <iostream>

int main() {
    MidiParser reader("../../Example/assets/vultureville.mid", MidiParser::Mode::Read);

    std::cout << reader.GetTrackCount() << std::endl;

    std::cout << std::hex;
    for (MidiTrack track : reader)
        for (MidiEvent event : track)
            std::cout << (int)event.Type << " " << (int)event.DataA << " " << (int)event.DataB << "\n";
    std::cout << std::dec;
}
