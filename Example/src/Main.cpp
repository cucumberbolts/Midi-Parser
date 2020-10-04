#include <MidiParser/MidiParser.h>

#include <iostream>

int main() {
    MidiParser parser("../../Example/assets/mapleleaf7.mid", MidiParser::Mode::Read);

    std::cout << "Track 1 size in bytes: " << parser[1].SizeInBytes() << "\n";
}
