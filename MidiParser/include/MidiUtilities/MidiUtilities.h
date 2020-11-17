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
    static std::string ConvertNote(MidiEvent* event) {
        // If event type is a note and velocity is greater than 0
        if (event->Type() == MidiEventType::NoteOn && event->DataB > 0) {
            std::string note(notes[event->DataA % 12]);
            note.append(1, (char)(event->DataA / 12 + ('0' - 1)));  // Append the octave number
            return note;
        }
        return {};
    }
};
