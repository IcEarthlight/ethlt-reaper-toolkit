#pragma once
#include "config.h"
#include <WDL/wdltypes.h> // might be unnecessary in future
#include "reaper_plugin_functions.h"
#include <string>

#include "actions/smart_midi_vel_adjust.h"

namespace PROJECT_NAME
{

namespace
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

constexpr double db2factor(double db) noexcept {
    double exp = db / 6;
    int int_part = static_cast<int>(exp);
    double frac_part = exp - int_part;
    
    // calculate 2^int_part using bit shifting
    double result = 1;
    if (int_part > 0)
        result *= static_cast<double>(1 << int_part);
    else if (int_part < 0)
        result /= static_cast<double>(1 << -int_part);
    
    // for fractional part, use a simple approximation
    if (frac_part != 0) {
        double x = frac_part;
        result *= 1.0 + 0.6931471805599453 * x 
                      + 0.24022650695910072 * x*x
                      + 0.05550410866482158 * x*x*x
                      + 0.009618129107628477 * x*x*x*x
                      + 0.0013333558146428443 * x*x*x*x*x;
    }
    
    return result;
}

constexpr int MIN_VOL_DB = -48;
constexpr double MIN_VOL_FACTOR = db2factor(MIN_VOL_DB);
#define STEP_OFFSET (increase ? 1.4 : -0.4)

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

// adjust_type: 0: normal
//              1: volume
//              2: mute {0, 1}
constexpr inline bool extract_envelope_info(
    const char *str,
    const size_t len,
    char *env_type,
    double *min_val,
    double *max_val,
    double *mid_val,
    int *adjust_type)
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

        if (parsed >= 1) {
            sprintf(env_type, "PARMENV %s", param_name);

            // try to find "DEFSHAPE" in str
            double defshape = 0;
            const char* defshape_ptr = strstr(end + 1, "DEFSHAPE");
    
            if (defshape_ptr &&
                sscanf(defshape_ptr + 8, "%lf", &defshape) == 1 &&
                defshape == 1
            ) {
                *adjust_type = 2; return true;
            }
        }

        if (parsed < 4) return false;
        
        // for factorial volume parameters in build-in plugins
        if ((*min_val == 0 && *max_val == 2 && *mid_val == 1) ||
            (*min_val == 0 && *max_val == 1 && *mid_val == 0.25) ||
            (*min_val == 0 && *max_val == 4 && *mid_val == 1))
            *adjust_type = 1;
        else
            *adjust_type = 0;
        
        return true;
    
    } else if (strcmp(env_type, "VOLENV") == 0 ||
               strcmp(env_type, "VOLENV2") == 0 ||
               strcmp(env_type, "AUXVOLENV") == 0) {

        *min_val = 0.0; *max_val = 2.0; *mid_val = 1.0; *adjust_type = 1; return true;

    } else if (strcmp(env_type, "PANENV") == 0 ||
               strcmp(env_type, "PANENV2") == 0 ||
               strcmp(env_type, "AUXPANENV") == 0 ||
               strcmp(env_type, "WIDTHENV") == 0 ||
               strcmp(env_type, "WIDTHENV2") == 0) {

        *min_val = -1.0; *max_val = 1.0; *mid_val = 0.0; *adjust_type = 0; return true;

    } else if (strcmp(env_type, "MUTEENV") == 0 ||
               strcmp(env_type, "AUXMUTEENV") == 0) {

        *min_val = 0.0; *max_val = 1.0; *mid_val = 0.5; *adjust_type = 2; return true;

    } else if (strcmp(env_type, "VOLENV3") == 0) {

        *min_val = 0.0; *max_val = 1.0; *mid_val = 0.5; *adjust_type = 1; return true;
    }

    return false;
}

template<bool increase, bool is_fine>
double adjust_volume(const double vol) noexcept
{
    if (vol <= 0)
        return increase ? MIN_VOL_FACTOR : 0;

    if (!increase && vol < MIN_VOL_FACTOR)
        return 0;
    
    const double new_vol = exp2(
        is_fine ? floor(log2(vol) * 12 + STEP_OFFSET) / 12 :
                  floor(log2(vol) *  2 + STEP_OFFSET) /  2
    );
    
    if (new_vol < MIN_VOL_FACTOR)
        return increase ? MIN_VOL_FACTOR : 0;
    
    return new_vol;
}

template<bool increase, bool is_fine>
double adjust_envpt_value(
    const double val,
    const double min_val,
    const double max_val,
    const double mid_val,
    const int adjust_type = 0
) noexcept
{
    static constexpr int STEP_NUM = is_fine ? 32 : 8;

    // common case
    if (adjust_type == 0 && min_val == 0 && max_val == 1 && mid_val == 0.5)
        return std::clamp(
            floor(STEP_NUM * val + STEP_OFFSET) / STEP_NUM,
            0.0, 1.0
        );
    
    if (adjust_type == 1 && min_val == 0)
        return std::min(max_val / mid_val, adjust_volume<increase, is_fine>(val / mid_val)) * mid_val;
    
    if (adjust_type == 2)
        return increase;
    
    // volume smoother
    if (min_val == -60 && max_val == 12 && mid_val == 0)
        return std::clamp(
            is_fine ? floor(val * 2 + STEP_OFFSET) / 2 :
                      floor(val / 3 + STEP_OFFSET) * 3,
            -60.0, 12.0
        );
    
    // normalize
    double factor = val < min_val ? 0 :
                    val < mid_val ? (val - min_val) / (mid_val - min_val) / 2 :
                    val < max_val ? (val - mid_val) / (max_val - mid_val) / 2 + 0.5 : 1;
    
    factor = floor(STEP_NUM * factor + STEP_OFFSET) / STEP_NUM;
    factor = std::clamp(factor, 0.0, 1.0);

    // scale back
    return factor < 0.5 ? min_val + (mid_val - min_val) * factor * 2 :
                          mid_val + (max_val - mid_val) * (factor - 0.5) * 2;
}

#undef STEP_OFFSET

template<bool increase, bool is_fine>
void adjust_item_volume(MediaItem *item)
{
    double vol = GetMediaItemInfo_Value(item, "D_VOL");
    SetMediaItemInfo_Value(item, "D_VOL", adjust_volume<increase, is_fine>(vol));
}

template<bool increase, bool is_fine>
void adjust_track_volume(MediaTrack *track)
{
    double vol = GetMediaTrackInfo_Value(track, "D_VOL");
    SetMediaTrackInfo_Value(track, "D_VOL", adjust_volume<increase, is_fine>(vol));
}

// simulate system volume key press
template<bool increase>
void adjust_system_volume()
{
#ifdef _WIN32
    // system volume adjustment on Windows
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = increase ? VK_VOLUME_UP : VK_VOLUME_DOWN;
    SendInput(1, &input, sizeof(INPUT)); // press key
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT)); // release key

#elif defined(__APPLE__)
    // UNTESTED: system volume adjustment on macOS
    #include <CoreGraphics/CoreGraphics.h>
    
    static constexpr int kVK_VolumeUp   = 0x48;
    static constexpr int kVK_VolumeDown = 0x49;
    
    // create key event
    CGEventRef event = CGEventCreateKeyboardEvent(
        nullptr,
        increase ? kVK_VolumeUp : kVK_VolumeDown,
        true  // keyDown
    );
    CGEventPost(kCGHIDEventTap, event); // press key
    CGEventSetType(event, kCGEventKeyUp);
    CGEventPost(kCGHIDEventTap, event); // release key
    
    CFRelease(event); // free event
#endif
}

template<bool increase, bool is_fine>
int adjust_all_selected_items_volume()
{
    int item_count = CountMediaItems(nullptr);
    int modified_count = 0;
    for (int i = 0; i < item_count; i++) {
        MediaItem *item = GetMediaItem(nullptr, i);
        if (IsMediaItemSelected(item)) {
            adjust_item_volume<increase, is_fine>(item);
            modified_count++;
        }
    }
    return modified_count;
}

template<bool increase, bool is_fine>
int adjust_all_selected_tracks_volume()
{
    int track_count = CountTracks(nullptr);
    int modified_count = 0;
    for (int i = 0; i < track_count; i++) {
        MediaTrack *track = GetTrack(nullptr, i);
        if (IsTrackSelected(track)) {
            adjust_track_volume<increase, is_fine>(track);
            modified_count++;
        }
    }
    return modified_count;
}

template<bool increase, bool is_fine>
int adjust_all_selected_envpoints_value(TrackEnvelope* env, char* env_type)
{
    int point_count = CountEnvelopePoints(env);
    int modified_count = 0;
    for (int i = 0; i < point_count; i++) {
        double point_val;
        bool selected;
        if (!GetEnvelopePoint(env, i, nullptr, &point_val, nullptr, nullptr, &selected))
            continue;
        if (!selected)
            continue;
        
        char env_state_chunk[256];
        if (!GetEnvelopeStateChunk(env, env_state_chunk, sizeof(env_state_chunk), true))
            continue;
        
        int adjust_type;
        double min_val, max_val, mid_val;
        if (!extract_envelope_info(env_state_chunk, strlen(env_state_chunk), env_type, &min_val, &max_val, &mid_val, &adjust_type))
            continue;
        
        static bool nosort = true;
        int scale_mode = GetEnvelopeScalingMode(env);
        double scaled_val = ScaleFromEnvelopeMode(scale_mode, point_val);
        scaled_val = adjust_envpt_value<increase, is_fine>(scaled_val, min_val, max_val, mid_val, adjust_type);
        point_val = ScaleToEnvelopeMode(scale_mode, scaled_val);
        modified_count += SetEnvelopePoint(env, i, nullptr, &point_val, nullptr, nullptr, &selected, &nosort);
    }

    if (modified_count)
        Envelope_SortPoints(env);

    return modified_count;
}

template<bool increase, bool is_fine>
int handle_arrange_view(int* modified_count, char* env_type)
{
    int ptrx, ptry;
    GetMousePosition(&ptrx, &ptry);
    *modified_count = 0;

    // try to handle items
    MediaItem *item = GetItemFromPoint(ptrx, ptry, true, nullptr);
    if (item && IsMediaItemSelected(item)) {
        *modified_count = adjust_all_selected_items_volume<increase, is_fine>();
        return 2;
    }

    // try to find tracks
    char thing[12];
    MediaTrack *track = GetThingFromPoint(ptrx, ptry, thing, sizeof(thing));
    if (!track) {
        // try to handle MIDI editor
        if (!try_to_handle_midi_editor<increase, is_fine>())
            adjust_system_volume<increase>();
        *modified_count = 0;
        return 0;
    }
    
    // try to handle env points
    if (strncmp(thing, "envelope", 8) == 0 || strncmp(thing, "envcp", 5) == 0) {
        int envidx = extract_index(thing, strlen(thing));
        TrackEnvelope* env = GetTrackEnvelope(track, envidx);
        if (env) {
            *modified_count = adjust_all_selected_envpoints_value<increase, is_fine>(env, env_type);
            if (*modified_count) return 3;
        }
    }

    // try to handle single/selected tracks
    if (IsTrackSelected(track)) {
        *modified_count = adjust_all_selected_tracks_volume<increase, is_fine>();
    } else {
        adjust_track_volume<increase, is_fine>(track);
        *modified_count = 1;
    }
    return 1;
}

} // anonymous namespace

// Adjusts selected media items' or tracks' volume based on template parameter
// @tparam increase If true, increases volume; if false, decreases volume
template<bool increase, bool is_fine>
void smart_vol_adjust()
{
    PreventUIRefresh(1);
    int modified_count;
    char env_type[64];
    // 0: nothing,1: tracks, 2: items, 3: envelope points
    int modified_class = handle_arrange_view<increase, is_fine>(&modified_count, env_type);

    if (modified_count > 0) {
        Undo_OnStateChange((
            (is_fine ? (increase ? "Slightly increase " : "Slightly decrease ") :
                       (increase ? "Increase " : "Decrease ")) +
            std::to_string(modified_count) +
            ( modified_class == 1 ?
                (std::string(modified_count == 1 ? " Track" : " Tracks") + " Volume") :
              modified_class == 2 ?
                (std::string(modified_count == 1 ? " Item" : " Items") + " Volume") :
              modified_class == 3 ?
                (std::string(modified_count == 1 ? " Envelope Point" : " Envelope Points") +
                " Value from " + env_type) :
              "[Unknown]" )
        ).c_str());
    }
    PreventUIRefresh(-1);
    UpdateArrange();
}

} // namespace PROJECT_NAME
