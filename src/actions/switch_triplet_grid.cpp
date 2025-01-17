#include "switch_triplet_grid.h"
#include <string>

namespace PROJECT_NAME
{

namespace
{

void switch_midi_grid()
{
    HWND midi_editor = MIDIEditor_GetActive();
    if (!midi_editor) return;

    MediaItem_Take *take = MIDIEditor_GetTake(midi_editor);
    if (!take) return;

    ShowConsoleMsg(("Grid: " + std::to_string(MIDI_GetGrid(take, nullptr, nullptr)) + "\n").c_str());
}

} // anonymous namespace

void switch_tripet_grid()
{

}

} // namespace PROJECT_NAME
