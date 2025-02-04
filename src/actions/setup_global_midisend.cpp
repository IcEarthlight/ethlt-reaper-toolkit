#include "setup_global_midisend.h"
#include <string>
#include <vector>
#include <cstdint>

namespace PROJECT_NAME
{

namespace
{

// void show_trackfx_params(MediaTrack *track, int fx_index)
// {
//     int param_count = TrackFX_GetNumParams(track, fx_index);
//     for (int i = 0; i < param_count; i++) {
//         double value = TrackFX_GetParam(track, fx_index, i, nullptr, nullptr);
//         char param_name[256];
//         TrackFX_GetParamName(track, fx_index, i, param_name, sizeof(param_name));
//         char formatted_value[256]; 
//         TrackFX_FormatParamValue(track, fx_index, i, value, formatted_value, sizeof(formatted_value));
        
//         ShowConsoleMsg(("Parameter " + std::to_string(i) + ": " + param_name + 
//                        " = " + std::to_string(value) + 
//                        " (" + formatted_value + ")\n").c_str());
//     }
// }

void init_global_midisend_track(MediaTrack *send_track)
{
    int fx_index = TrackFX_AddByName(send_track, "VST: ReaControlMIDI (Cockos)", false, -1);
    if (fx_index == -1) return;

    TrackFX_SetPreset(send_track, fx_index, "global midisend");
    GetSetMediaTrackInfo_String(send_track, "P_NAME", "[Global MIDI Send]", true);
    GetSetMediaTrackInfo_String(send_track, "P_EXT:is_global_midisend", "true", true);
    GetFXEnvelope(send_track, fx_index, 3, true);
}

bool is_valid_midisend_track(MediaTrack *track)
{
    char ext_data[6];

    if (track &&
        // check if the cached track pointer is still valid
        ValidatePtr2(nullptr, track, "MediaTrack*") &&
        // check if the track has the global midisend flag
        GetSetMediaTrackInfo_String(track, "P_EXT:is_global_midisend", ext_data, false) &&
        ext_data[0] == 't'
    ) {
        // check if ReaControlMIDI exists in track FX chain
        if (TrackFX_AddByName(track, "VST: ReaControlMIDI (Cockos)", false, 0) != -1) {
            return true;
        } else {
            GetSetMediaTrackInfo_String(track, "P_EXT:is_global_midisend", "false", true);
            return false;
        }
    } else {
        return false;
    }
}

MediaTrack *get_or_create_global_midisend_track()
{
    static MediaTrack *send_track = nullptr;
    
    if (is_valid_midisend_track(send_track))
        return send_track;

    int track_count = CountTracks(nullptr);
    for (int i = 0; i < track_count; i++)
    {
        MediaTrack *track = GetTrack(nullptr, i);
        if (track && is_valid_midisend_track(track)) {
            send_track = track;
            return send_track;
        }
    }

    InsertTrackInProject(nullptr, track_count, false);
    send_track = GetTrack(nullptr, track_count);
    init_global_midisend_track(send_track);

    return send_track;
}

// create sends to all tracks except self and those already receiving
void send_to_all_tracks(MediaTrack *send_track)
{
    int track_count = CountTracks(nullptr);
    for (int i = 0; i < track_count; i++) {
        MediaTrack *track = GetTrack(nullptr, i);
        if (!track) continue;
        
        // skip if it's the source track
        if (track == send_track) continue;
        
        // check if the track already receives from send_track
        int recv_count = GetTrackNumSends(track, -1); // -1 for receives
        bool already_linked = false;
        for (int j = 0; j < recv_count; j++) {
            MediaTrack* src_track = reinterpret_cast<MediaTrack*>(
                static_cast<uintptr_t>(GetTrackSendInfo_Value(track, -1, j, "P_SRCTRACK"))
            );
            if (src_track == send_track) {
                already_linked = true;
                break;
            }
        }
        if (already_linked) continue;

        // create new send
        int send_idx = CreateTrackSend(send_track, track);
        if (send_idx >= 0) {
            ShowConsoleMsg(("Created send to track " + std::to_string(i) + "\n").c_str());
            SetTrackSendInfo_Value(send_track, 0, send_idx, "I_SRCCHAN", -1);  // no audio send
            SetTrackSendInfo_Value(send_track, 0, send_idx, "I_MIDIFLAGS", 0x84);  // send MIDI channel 4 -> 4
        }
    }
}

} // anonymous namespace

void setup_global_midisend()
{
    MediaTrack *send_track = get_or_create_global_midisend_track();
    if (!send_track) return;

    send_to_all_tracks(send_track);
}

} // namespace PROJECT_NAME