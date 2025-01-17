#include "test.h"
#include <stdio.h>
#include <string>

namespace PROJECT_NAME
{

constexpr inline int ipow(int base, int exp) noexcept {
    int result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

constexpr inline int extract_index(const char* str, const size_t len) noexcept
{
    int n = 0;
    for (int i = 0; i < len; i++) {
        char c = str[len - i - 1];
        if (isdigit(c))
            n += (c - '0') * ipow(10, i);
        else break;
    }
    return n;
}

constexpr inline bool extract_envelope_info(
    const char *str,
    const size_t len,
    char *env_type,
    double *min_val,
    double *max_val,
    double *mid_val
) noexcept
{
    if (len < 1 || str[0] != '<')
        return false;

    // find first space
    const char *end = str;
    while (*end && !isspace(*end)) end++;
    if (!*end) return false;

    // assign env_type
    size_t env_type_len = end - str - 1;
    strncpy(env_type, str + 1, env_type_len);
    env_type[env_type_len] = '\0';

    if (strcmp(env_type, "PARMENV") == 0) {
        char param_name[32] = {};
        int parsed = sscanf(end + 1, "%s %lf %lf %lf", param_name, min_val, max_val, mid_val);
        sprintf(env_type, "PARMENV %s", param_name);
        return parsed == 4;
    
    } else if (strcmp(env_type, "VOLENV") == 0 ||
               strcmp(env_type, "VOLENV2") == 0 ||
               strcmp(env_type, "AUXVOLENV") == 0) {

        *min_val = 0.0; *max_val = 2.0; *mid_val = 1.0; return true;

    } else if (strcmp(env_type, "PANENV") == 0 ||
               strcmp(env_type, "PANENV2") == 0 ||
               strcmp(env_type, "AUXPANENV") == 0 ||
               strcmp(env_type, "WIDTHENV") == 0 ||
               strcmp(env_type, "WIDTHENV2") == 0) {

        *min_val = -1.0; *max_val = 1.0; *mid_val = 0.0; return true;

    } else if (strcmp(env_type, "MUTEENV") == 0 ||
               strcmp(env_type, "VOLENV3") == 0 ||
               strcmp(env_type, "AUXMUTEENV") == 0) {

        *min_val = 0.0; *max_val = 1.0; *mid_val = 0.5; return true;

    }

    return false;
}

bool active_midi_editor() {
    HWND hwnd = MIDIEditor_GetActive();
    if (hwnd == nullptr) {
        return false;
    }
    MediaItem_Take *take = MIDIEditor_GetTake(hwnd);
    if (take == nullptr) {
        return false;
    }
    return true;
}

// helper function to get element under cursor
void print_element_under_point(int x, int y) {
    char buf[1024];
    sprintf(buf, "Cursor position: %d, %d\n", x, y);
    ShowConsoleMsg(buf);
    buf[0] = '\0'; // clear buf

    char info[256];
    GetMousePosition(&x, &y);
    MediaTrack *track = GetThingFromPoint(x, y, info, sizeof(info));
    if (track) {
        char trackName[256];
        GetTrackName(track, trackName, sizeof(trackName));
        sprintf(buf, "GetThingFromPoint: %s %s\n", trackName, info);
        if (strncmp(info, "envelope", 8) == 0 || strncmp(info, "envcp", 5) == 0) {
            int envidx = extract_index(info, strlen(info));
            ShowConsoleMsg(("envidx: " + std::to_string(envidx) + "\n").c_str());
            TrackEnvelope* env = GetTrackEnvelope(track, envidx);
            if (env) {
                char env_state_chunk[256];
                if (GetEnvelopeStateChunk(env, env_state_chunk, sizeof(env_state_chunk), true)) {
                    // char env_type[32];
                    // double min_val, max_val, mid_val;
                    // if (extract_envelope_info(env_state_chunk, strlen(env_state_chunk), env_type, &min_val, &max_val, &mid_val)) {
                    //     ShowConsoleMsg(("env_type: " + std::string(env_type) + "\n").c_str());
                    //     ShowConsoleMsg(("min_val: " + std::to_string(min_val) + "\n").c_str());
                    //     ShowConsoleMsg(("max_val: " + std::to_string(max_val) + "\n").c_str());
                    //     ShowConsoleMsg(("mid_val: " + std::to_string(mid_val) + "\n").c_str());
                    // } else {
                    //     ShowConsoleMsg("Failed to extract envelope info:\n");
                    //     ShowConsoleMsg(env_state_chunk);
                    // }
                    ShowConsoleMsg(env_state_chunk);
                }
                int point_count = CountEnvelopePoints(env);
                for (int i = 0; i < point_count; i++) {
                    double point_val;
                    bool selected;
                    if (GetEnvelopePoint(env, i, nullptr, &point_val, nullptr, nullptr, &selected) && selected) {
                        int scale_mode = GetEnvelopeScalingMode(env);
                        double scaled_val = ScaleFromEnvelopeMode(scale_mode, point_val);
                        ShowConsoleMsg(("selected point: " + std::to_string(i) + " " + std::to_string(point_val) + " " + std::to_string(scale_mode) + " " + std::to_string(scaled_val) + "\n").c_str());
                    }
                }
            }
        }
    } else {
        sprintf(buf, "GetThingFromPoint: non-track %s\n", info);
    }
    ShowConsoleMsg(buf[0] == '\0' ? "unknown\n" : buf);
    buf[0] = '\0'; // clear buf

    int info_out = 0;
    track = GetTrackFromPoint(x, y, &info_out);
    if (track) {
        char trackName[256];
        GetTrackName(track, trackName, sizeof(trackName));
        sprintf(buf, "GetTrackFromPoint: %s %d\n", trackName, info_out);
    } else {
        sprintf(buf, "GetTrackFromPoint: non-track %d\n", info_out);
    }
    ShowConsoleMsg(buf[0] == '\0' ? "unknown\n" : buf);
    buf[0] = '\0'; // clear buf

    MediaItem_Take *take = nullptr;
    MediaItem *item = GetItemFromPoint(x, y, true, &take);
    sprintf(buf, "GetItemFromPoint:\n");
    if (item) {
        GetSetMediaItemInfo_String(item, "P_NOTES", info, false);
        sprintf(buf, "%sItemInfo: P_NOTES %s\n", buf, info);
        GetSetMediaItemInfo_String(item, "P_EXT", info, false);
        sprintf(buf, "%s          P_EXT %s\n", buf, info);
        GetSetMediaItemInfo_String(item, "GUID", info, false);
        sprintf(buf, "%s          GUID %s\n", buf, info);
        double pos = GetMediaItemInfo_Value(item, "D_POSITION");
        sprintf(buf, "%s          D_POSITION %f\n", buf, pos);
        double vol = GetMediaItemInfo_Value(item, "D_VOL");
        sprintf(buf, "%s          D_VOL %f\n", buf, vol);
    } else {
        sprintf(buf, "%sItemInfo: None\n", buf);
    }
    if (take) {
        GetSetMediaItemTakeInfo_String(take, "P_NAME", info, false);
        sprintf(buf, "%sTakeInfo: P_NAME %s\n", buf, info);
        GetSetMediaItemTakeInfo_String(take, "P_EXT", info, false);
        sprintf(buf, "%s          P_EXT %s\n", buf, info);
        GetSetMediaItemTakeInfo_String(take, "GUID", info, false);
        sprintf(buf, "%s          GUID %s\n", buf, info);
        double vol = GetMediaItemTakeInfo_Value(take, "D_VOL");
        sprintf(buf, "%s          D_VOL %f\n", buf, vol);
    } else {
        sprintf(buf, "%sTakeInfo: None\n", buf);
    }
    ShowConsoleMsg(buf[0] == '\0' ? "unknown\n" : buf);
    buf[0] = '\0'; // clear buf
}

void test()
{
    POINT pt;
    if (!GetCursorPos(&pt)) {
        ShowConsoleMsg("Failed to get cursor position\n");
        return;
    }
    if (active_midi_editor()) {
        ShowConsoleMsg("MIDI Editor is active\n");
    }
    print_element_under_point(pt.x, pt.y);
}

void show_selected_midi_items() {
    HWND midi_editor = MIDIEditor_GetActive();
    if (!midi_editor) return;

    MediaItem_Take *take = MIDIEditor_GetTake(midi_editor);
    if (!take) return;

    ShowConsoleMsg("Selected MIDI items:\n");

    double end_pos = -HUGE_VAL;
    int note_count;
    MIDI_CountEvts(take, &note_count, nullptr, nullptr);

    for (int i = 0; i < note_count; i++) {
        bool selected, muted;
        double note_start_pos, note_end_pos;
        int channel, pitch, velocity;
        bool result = MIDI_GetNote(take, i, &selected, &muted, &note_start_pos, &note_end_pos, &channel, &pitch, &velocity);

        if (!result) continue;
        if (!selected) continue;
        
        ShowConsoleMsg((
            "note_index: " + std::to_string(i) +
            "\n  note_pos: " + std::to_string(note_start_pos) + " - " + std::to_string(note_end_pos) +
            "\n  channel: " + std::to_string(channel) +
            " pitch: " + std::to_string(pitch) +
            " velocity: " + std::to_string(velocity) +
            "\n"
        ).c_str());
    }
}

void show_all_midi_items() {
    HWND midi_editor = MIDIEditor_GetActive();
    if (!midi_editor) return;

    MediaItem_Take *take = MIDIEditor_GetTake(midi_editor);
    if (!take) return;

    ShowConsoleMsg("All MIDI items:\n");

    double end_pos = -HUGE_VAL;
    int note_count;
    MIDI_CountEvts(take, &note_count, nullptr, nullptr);

    for (int i = 0; i < note_count; i++) {
        bool selected, muted;
        double note_start_pos, note_end_pos;
        int channel, pitch, velocity;
        bool result = MIDI_GetNote(take, i, &selected, &muted, &note_start_pos, &note_end_pos, &channel, &pitch, &velocity);

        if (!result) continue;
        
        ShowConsoleMsg((
            "note_index: " + std::to_string(i) + (selected ? "[selected] " : "") +
            "\n  note_pos: " + std::to_string(note_start_pos) + " - " + std::to_string(note_end_pos) +
            "\n  channel: " + std::to_string(channel) +
            " pitch: " + std::to_string(pitch) +
            " velocity: " + std::to_string(velocity) +
            "\n"
        ).c_str());
    }
}

}