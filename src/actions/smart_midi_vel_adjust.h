#pragma once
#include "config.h"
#include <WDL/wdltypes.h> // might be unnecessary in future
#include "reaper_plugin_functions.h"
#include <string>

namespace PROJECT_NAME
{

namespace
{

inline int increase_velocity(int vel) noexcept
{
    return std::min(0x7F, (((vel+8) >> 4) + 1) << 4);
}

inline int decrease_velocity(int vel) noexcept
{
    return std::max(1, (((vel+8) >> 4) - 1) << 4);
}

inline int handle_midi_editor(bool increase)
{
    HWND midi_editor = MIDIEditor_GetActive();
    if (!midi_editor) return -1;

    MediaItem_Take *take = MIDIEditor_GetTake(midi_editor);
    if (!take) return -1;

    int modified_notes_count = 0;
    int notecnt, ccevtcnt, textsyxevtcnt;
    MIDI_CountEvts(take, &notecnt, &ccevtcnt, &textsyxevtcnt);

    static constexpr bool NOSORT_TRUE = true;
    for (int i = 0; i < notecnt; i++) {
        bool selected;
        int vel;
        bool result = MIDI_GetNote(take, i, &selected, nullptr, nullptr, nullptr, nullptr, nullptr, &vel);

        if (!result) continue;
        if (!selected) continue;

        modified_notes_count++;
        int new_vel = (increase ? increase_velocity : decrease_velocity)(vel);
        MIDI_SetNote(take, i, &selected, nullptr, nullptr, nullptr, nullptr, nullptr, &new_vel, &NOSORT_TRUE);
    }

    if (modified_notes_count) {
        MIDI_Sort(take);
        ShowConsoleMsg((std::to_string(modified_notes_count) + " " + (modified_notes_count == 1 ? "note" : "notes") + " " + (increase ? "increased" : "decreased") + " velocity\n").c_str());
    } else {
        ShowConsoleMsg("No notes selected\n");
    }

    return modified_notes_count;
}

} // anonymous namespace

template<bool increase>
bool try_to_handle_midi_editor()
{
    int modified_notes_count = handle_midi_editor(increase);
    if (0 < modified_notes_count) {
        Undo_OnStateChange((
            (increase ? "Increase " : "Decrease ") + 
            std::to_string(modified_notes_count) + 
            " MIDI " +
            (modified_notes_count == 1 ? "Note" : "Notes") +
            " Velocity"
        ).c_str());
        return true;
    }
    return false;
}

// Adjusts selected MIDI notes' velocity in the active MIDI editor (if any)
// @tparam increase If true, increases velocity; if false, decreases velocity
template<bool increase>
void smart_midi_vel_adjust()
{
    PreventUIRefresh(1);
    int modified_notes_count = handle_midi_editor(increase);
    if (0 < modified_notes_count) {
        Undo_OnStateChange((
            (increase ? "Increase " : "Decrease ") + 
            std::to_string(modified_notes_count) + 
            " MIDI " +
            (modified_notes_count == 1 ? "Note" : "Notes") +
            " Velocity"
        ).c_str());
    } else if (modified_notes_count == -1) {
        ShowConsoleMsg("No MIDI editor found\n");
    }
    PreventUIRefresh(-1);
    UpdateArrange();
}

} // namespace PROJECT_NAME
