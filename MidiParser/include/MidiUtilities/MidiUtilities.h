#pragma once

#include <string>

#include "MidiEvent.h"

class MidiUtilities {
private:
    static constexpr const char* notes[] = {
        "C",
        "Db/C#",
        "D",
        "Eb/D#",
        "E",
        "F",
        "Gb/F#",
        "G",
        "Ab/G#",
        "A",
        "Bb/A#",
        "B"
    };
public:
    static std::string NoteToString(MidiEvent* event) {
        if (event->GetType() == MidiEventType::NoteOn) {
            std::string note(notes[event->GetDataA() % 12]);
            note.append(1, (char)(event->GetDataA() / 12 + ('0' - 1)));  // Append the octave number
            return note;
        }
        return "";
    }
};
