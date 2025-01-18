#include "append_duplicate.h"
#include <string>
#include <utility>
#include <vector>

namespace PROJECT_NAME
{

namespace
{

struct MIDINote
{
    bool selected, muted;
    double start_ppq, end_ppq;
    int channel, pitch, velocity;
};

static int handle_midi_editor()
{
    HWND midi_editor = MIDIEditor_GetActive();
    if (!midi_editor) return 0;

    MediaItem_Take *take = MIDIEditor_GetTake(midi_editor);
    if (!take) return 0;

    int note_count;
    std::vector<MIDINote> selected_notes;
    double start_ppq = HUGE_VAL, end_ppq = -HUGE_VAL;
    MIDI_CountEvts(take, &note_count, nullptr, nullptr);
    static constexpr bool SELECTED_FALSE = false;
    static constexpr bool NOSORT_TRUE = true;

    for (int i = 0; i < note_count; i++) {
        MIDINote note = {};
        bool result = MIDI_GetNote(take, i, &note.selected, &note.muted,
                                   &note.start_ppq, &note.end_ppq, &note.channel,
                                   &note.pitch, &note.velocity);

        if (!result) continue;
        if (!note.selected) continue;

        selected_notes.push_back(note);
        start_ppq = std::min(start_ppq, note.start_ppq);
        end_ppq = std::max(end_ppq, note.end_ppq);

        // deselect note
        MIDI_SetNote(take, i, &SELECTED_FALSE, &note.muted,
                     &note.start_ppq, &note.end_ppq, &note.channel,
                     &note.pitch, &note.velocity, &NOSORT_TRUE);
    }

    if (selected_notes.empty()) return 0;

    double interval = end_ppq - start_ppq;

    for (const MIDINote &note : selected_notes) {
        MIDI_InsertNote(take, note.selected, note.muted,
                        note.start_ppq + interval, note.end_ppq + interval,
                        note.channel, note.pitch, note.velocity, &NOSORT_TRUE);
    }

    MIDI_Sort(take);

    return static_cast<int>(selected_notes.size());
}

static int handle_arrange_view()
{
    double end_pos = -HUGE_VAL;
    int min_track_id = INT_MAX;
    int selected_item_count = 0;
    double cur_pos = GetCursorPosition();

    int item_count = CountMediaItems(nullptr);
    for (int i = 0; i < item_count; i++) {
        MediaItem *item = GetMediaItem(nullptr, i);
        if (IsMediaItemSelected(item)) {
            double pos = GetMediaItemInfo_Value(item, "D_POSITION");
            double len = GetMediaItemInfo_Value(item, "D_LENGTH");
            int track_id = (int)GetMediaItemInfo_Value(item, "IP_TRACKNUMBER");
            end_pos = std::max(end_pos, pos + len);
            min_track_id = std::min(min_track_id, track_id);
            selected_item_count++;
        }
    }

    if (selected_item_count == 0) return 0;

    // deselect all tracks
    int track_count = CountTracks(nullptr);
    std::vector<MediaTrack*> selected_tracks;
    for (int i = 0; i < track_count; i++) {
        MediaTrack* track = GetTrack(nullptr, i);
        if (IsTrackSelected(track))
            selected_tracks.push_back(track);
        SetTrackSelected(track, false);
    }

    Main_OnCommand(40057, 0); // Item: Copy items/tracks
    SetEditCurPos(end_pos, false, false);
    MediaTrack* track = GetTrack(nullptr, min_track_id);
    SetOnlyTrackSelected(track);
    Main_OnCommand(42398, 0); // Item: Paste items/tracks

    // reposition cursor and reselect all tracks
    SetEditCurPos(cur_pos, false, false);
    for (MediaTrack *track : selected_tracks)
        SetTrackSelected(track, true);

    return selected_item_count;
}

} // anonymous namespace

void append_duplicate_midi_editor()
{
    PreventUIRefresh(1);
    if (int n = handle_midi_editor())
        Undo_OnStateChange((
            "Append Duplicate " +
            std::to_string(n) +
            " MIDI " +
            (n == 1 ? "Note" : "Notes")
        ).c_str());
    PreventUIRefresh(-1);
    UpdateArrange();
}

void append_duplicate_main()
{
    PreventUIRefresh(1);
    if (int n = handle_arrange_view()) {
        Undo_OnStateChange((
            "Append Duplicate " +
            std::to_string(n) +
            (n == 1 ? " Item" : " Items")
        ).c_str());
        ShowConsoleMsg(("Append Duplicate " + std::to_string(n) + (n == 1 ? "Item" : "Items")).c_str());
    }
    PreventUIRefresh(-1);
    UpdateArrange();
}

} // namespace PROJECT_NAME
