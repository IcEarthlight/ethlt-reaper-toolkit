#include "switch_triplet_grid.h"
#include <string>

namespace PROJECT_NAME
{

namespace
{

static constexpr double switch_gridsize(const double gridsize) noexcept
{
    unsigned int denominator = (unsigned int)(1 / gridsize + 0.5);

    if (denominator % 3) // not dividable by 3
        if (denominator % 2)
            return gridsize;
        else
            return 1. / (denominator / 2 * 3);
    else
        return 1. / (denominator / 3 * 2);
}

} // anonymous namespace

void switch_triplet_main_grid()
{
    double gridsize;
    GetSetProjectGrid(nullptr, false, &gridsize, nullptr, nullptr);
    gridsize = switch_gridsize(gridsize);
    SetProjectGrid(nullptr, gridsize);
}

void switch_triplet_midi_grid()
{
    HWND midi_editor = MIDIEditor_GetActive();
    if (!midi_editor) return;

    MediaItem_Take *take = MIDIEditor_GetTake(midi_editor);
    if (!take) return;

    double gridsize = MIDI_GetGrid(take, nullptr, nullptr) / 4;
    gridsize = switch_gridsize(gridsize);
    SetMIDIEditorGrid(nullptr, gridsize);
}

} // namespace PROJECT_NAME
