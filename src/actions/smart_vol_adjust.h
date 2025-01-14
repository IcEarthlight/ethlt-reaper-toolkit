#pragma once
#include "config.h"
#include <WDL/wdltypes.h> // might be unnecessary in future
#include "reaper_plugin_functions.h"
#include <string>

namespace PROJECT_NAME
{

namespace
{

inline double increase_volume(double vol) noexcept
{
    double log_vol = log2(vol) / 2;
    return pow(sqrt(2), floor(log_vol + 1.5));
}

inline double decrease_volume(double vol) noexcept
{
    double log_vol = log2(vol) / 2;
    return pow(sqrt(2), floor(log_vol - 0.5));
}

inline void increase_track_volume(MediaTrack *track)
{
    double vol = GetMediaTrackInfo_Value(track, "D_VOL");
    SetMediaTrackInfo_Value(track, "D_VOL", increase_volume(vol));
}

inline void decrease_track_volume(MediaTrack *track)
{
    double vol = GetMediaTrackInfo_Value(track, "D_VOL");
    SetMediaTrackInfo_Value(track, "D_VOL", decrease_volume(vol));
}

inline void increase_item_volume(MediaItem *item)
{
    double vol = GetMediaItemInfo_Value(item, "D_VOL");
    SetMediaItemInfo_Value(item, "D_VOL", increase_volume(vol));
}

inline void decrease_item_volume(MediaItem *item)
{
    double vol = GetMediaItemInfo_Value(item, "D_VOL");
    SetMediaItemInfo_Value(item, "D_VOL", decrease_volume(vol));
}

} // anonymous namespace

// Adjusts selected media items' or tracks' volume based on template parameter
// @tparam increase If true, increases volume; if false, decreases volume
template<bool increase>
void SmartVolAdjust()
{
    PreventUIRefresh(1);
    // TODO: implement
    // if (0 < modified_notes_count) {
    //     Undo_OnStateChange(("Increase " + std::to_string(modified_notes_count) + " MIDI Notes Velocity").c_str());
    PreventUIRefresh(-1);
    UpdateArrange();
}

} // namespace PROJECT_NAME
