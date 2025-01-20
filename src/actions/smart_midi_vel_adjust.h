#pragma once
#include "config.h"
#include <WDL/wdltypes.h> // might be unnecessary in future
#include "reaper_plugin_functions.h"
#include <string>

namespace PROJECT_NAME
{

namespace
{

template<bool increase, bool is_fine>
inline int adjust_velocity(int vel) noexcept
{
    if (is_fine)
        return increase ? std::min(0x7F, (((vel+1) >> 2) + 1) << 2) :
                          std::max(1,    (((vel+3) >> 2) - 1) << 2);
    else
        return increase ? std::min(0x7F, (((vel+7) >> 4) + 1) << 4) :
                          std::max(1,    (((vel+9) >> 4) - 1) << 4);
}

template<bool increase, bool is_fine>
int handle_midi_editor()
{
    HWND midi_editor = MIDIEditor_GetActive();
    if (!midi_editor) return 0;

    MediaItem_Take *take = MIDIEditor_GetTake(midi_editor);
    if (!take) return 0;

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
        int new_vel = adjust_velocity<increase, is_fine>(vel);
        MIDI_SetNote(take, i, &selected, nullptr, nullptr, nullptr, nullptr, nullptr, &new_vel, &NOSORT_TRUE);
    }

    if (modified_notes_count)
        MIDI_Sort(take);

    return modified_notes_count;
}

} // anonymous namespace

// Adjusts selected MIDI notes' velocity in the active MIDI editor (if any)
// @tparam increase If true, increases velocity; if false, decreases velocity
template<bool increase, bool is_fine>
void smart_midi_vel_adjust()
{
    PreventUIRefresh(1);
    
    if (int n = handle_midi_editor<increase, is_fine>()) {
        Undo_OnStateChange((
            (is_fine ? (increase ? "Slightly increase " : "Slightly decrease ") :
                       (increase ? "Increase " : "Decrease ")) +
            std::to_string(n) +
            " MIDI " +
            (n == 1 ? "Note" : "Notes") +
            " Velocity"
        ).c_str());
    }
    PreventUIRefresh(-1);
    UpdateArrange();
}

} // namespace PROJECT_NAME
