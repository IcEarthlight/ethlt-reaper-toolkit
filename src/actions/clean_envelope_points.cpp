#include "clean_envelope_points.h"
#include <cmath>
#include <optional>
#include <stack>
#include <string>

namespace PROJECT_NAME
{

namespace
{

union reinterpretable_double
{
    double d;
    uint64_t i;
    struct {
        uint64_t value : 63;
        uint64_t sign : 1;
    };
    constexpr reinterpretable_double(double value) noexcept : d(value) { }
    // constexpr reinterpretable_double(uint64_t value) noexcept : i(value) { }
};

constexpr int IGNORE_LAST_N_BITS = 34;
constexpr inline bool almost_equal(reinterpretable_double a, reinterpretable_double b) noexcept
{
    if (a.d == b.d) return true;
    
    if (std::isfinite(a.d) && std::isfinite(b.d)) {
        const uint64_t diff = (a.sign == b.sign) ?
            (a.value > b.value) ?
                (a.value - b.value) :
                (b.value - a.value) :
            (a.value + b.value);
        
        return !(diff >> IGNORE_LAST_N_BITS);
    }

    return false;
}

struct EnvPoint
{
    double time, value;
    int shape;
};


static int handle_envelope(TrackEnvelope* env)
{
    int point_count, del_point_count = 0;
    std::stack<int> del_point_indices;
    std::optional<EnvPoint> last_point, second_last_point;

    int autoitem_count = CountAutomationItems(env);
    for (int i = -1; i < autoitem_count; i++) { // -1 is for underlying envelope

        // delete consecutive points with the same time
        point_count = CountEnvelopePointsEx(env, i);
        for (int j = 0; j < point_count; j++) {
            EnvPoint point;
            if (!GetEnvelopePointEx(env, i, j, &point.time, &point.value, &point.shape, nullptr, nullptr))
                continue;
            
            // ShowConsoleMsg((std::to_string(j) + ":\n" +
            //     "    second_last_point: " + (second_last_point.has_value() ? floatToString(second_last_point->time) : "NaN") + "\n"
            //     "    last_point:        " + (last_point.has_value() ? floatToString(last_point->time) : "NaN") + "\n" +
            //     "    point:             " + floatToString(point.time) + "\n" +
            //     (last_point.has_value() && second_last_point.has_value() ? (
            //         std::string("    last_time == time: ") + (almost_equal(last_point->time, point.time) ? "true" : "false") + "\n" +
            //         "    second_time == time: " + (almost_equal(second_last_point->time, last_point->time) ? "true" : "false") + "\n" ) : ""
            //     ) + "\n"
            // ).c_str());

            if (last_point.has_value() &&
                second_last_point.has_value() &&
                almost_equal(last_point->time, point.time) &&
                almost_equal(second_last_point->time, last_point->time)
            )
                del_point_indices.push(j-1);

            second_last_point = last_point;
            last_point = point;
        }

        del_point_count += static_cast<int>(del_point_indices.size());

        while (!del_point_indices.empty()) {
            DeleteEnvelopePointEx(env, i, del_point_indices.top());
            del_point_indices.pop();
        }

        last_point = std::nullopt;
        second_last_point = std::nullopt;

        // delete overlapping points
        point_count = CountEnvelopePointsEx(env, i);
        for (int j = 0; j < point_count; j++) {
            EnvPoint point;
            if (!GetEnvelopePointEx(env, i, j, &point.time, &point.value, &point.shape, nullptr, nullptr))
                continue;
            
            if (last_point.has_value() &&
                almost_equal(last_point->time, point.time) &&
                last_point->value == point.value
            )
                del_point_indices.push(j-1);

            last_point = point;
        }

        del_point_count += static_cast<int>(del_point_indices.size());

        while (!del_point_indices.empty()) {
            DeleteEnvelopePointEx(env, i, del_point_indices.top());
            del_point_indices.pop();
        }

        last_point = std::nullopt;
        second_last_point = std::nullopt;

        // delete consecutive points with the same value
        point_count = CountEnvelopePointsEx(env, i);
        for (int j = 0; j < point_count; j++) {
            EnvPoint point;
            if (!GetEnvelopePointEx(env, i, j, &point.time, &point.value, &point.shape, nullptr, nullptr))
                continue;

            if (last_point.has_value() &&
                second_last_point.has_value() &&
                last_point->value == point.value &&
                second_last_point->value == last_point->value
            )
                del_point_indices.push(j-1);

            second_last_point = last_point;
            last_point = point;
        }

        del_point_count += static_cast<int>(del_point_indices.size());

        while (!del_point_indices.empty()) {
            DeleteEnvelopePointEx(env, i, del_point_indices.top());
            del_point_indices.pop();
        }

        last_point = std::nullopt;
        second_last_point = std::nullopt;

        // delete unnecessary square points
        point_count = CountEnvelopePointsEx(env, i);
        for (int j = 0; j < point_count; j++) {
            EnvPoint point;
            if (!GetEnvelopePointEx(env, i, j, &point.time, &point.value, &point.shape, nullptr, nullptr))
                continue;
            
            if (last_point.has_value() &&
                last_point->shape == 1 &&
                point.shape == 1 &&
                last_point->value == point.value
            )
                del_point_indices.push(j);

            last_point = point;
        }

        del_point_count += static_cast<int>(del_point_indices.size());

        while (!del_point_indices.empty()) {
            DeleteEnvelopePointEx(env, i, del_point_indices.top());
            del_point_indices.pop();
        }

        last_point = std::nullopt;
        second_last_point = std::nullopt;

        // check if the tail point is necessary
        point_count = CountEnvelopePointsEx(env, i);
        if (point_count >= 2) {
            EnvPoint point, tail_point;
            if (GetEnvelopePointEx(env, i, point_count-2, &point.time, &point.value, &point.shape, nullptr, nullptr) &&
                GetEnvelopePointEx(env, i, point_count-1, &tail_point.time, &tail_point.value, &tail_point.shape, nullptr, nullptr) &&
                point.value == tail_point.value
            ) {
                DeleteEnvelopePointEx(env, i, point_count-1);
                del_point_count++;
            }
        }

        // check if the head point is necessary
        point_count = CountEnvelopePointsEx(env, i);
        if (point_count >= 2) {
            EnvPoint head_point, point;
            if (GetEnvelopePointEx(env, i, 0, &head_point.time, &head_point.value, &head_point.shape, nullptr, nullptr) &&
                GetEnvelopePointEx(env, i, 1, &point.time, &point.value, &point.shape, nullptr, nullptr) &&
                head_point.value == point.value
            ) {
                DeleteEnvelopePointEx(env, i, 0);
                del_point_count++;
            }
        }
    }

    if (del_point_count)
        Envelope_SortPoints(env);
    
    return del_point_count;
}

static int handle_all_track_envelopes()
{
    int del_point_count = 0;

    int track_count = CountTracks(nullptr);
    for (int i = 0; i < track_count; i++) {
        MediaTrack *track = GetTrack(nullptr, i);
        if (!track) continue;

        // handle track envelopes
        int env_count = CountTrackEnvelopes(track);
        for (int j = 0; j < env_count; j++) {
            TrackEnvelope *env = GetTrackEnvelope(track, j);
            if (!env) continue;
            del_point_count += handle_envelope(env);
        }

        // handle take envelopes
        int item_count = CountTrackMediaItems(track);
        for (int j = 0; j < item_count; j++) {
            MediaItem *item = GetTrackMediaItem(track, j);
            if (!item) continue;

            int take_count = CountTakes(item);
            for (int k = 0; k < take_count; k++) {
                MediaItem_Take *take = GetMediaItemTake(item, k);
                if (!take) continue;

                int take_env_count = CountTakeEnvelopes(take);
                for (int l = 0; l < take_env_count; l++) {
                    TrackEnvelope *env = GetTakeEnvelope(take, l);
                    if (!env) continue;
                    
                    del_point_count += handle_envelope(env);
                }
            }
        }
    }

    return del_point_count;
}

} // anonymous namespace

void clean_envelope_points()
{
    PreventUIRefresh(1);
    
    if (int n = handle_all_track_envelopes())
        Undo_OnStateChange((
            "Clean " + std::to_string(n) +
            (n == 1 ? " Envelope Point" : " Envelope Points")
        ).c_str());

    PreventUIRefresh(-1);
    UpdateArrange();
}

} // namespace PROJECT_NAME
