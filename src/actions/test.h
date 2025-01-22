#pragma once
#include "config.h"
#include <WDL/wdltypes.h> // might be unnecessary in future
#include "reaper_plugin_functions.h"
#include <sstream>
#include <iomanip>

namespace PROJECT_NAME
{

template<typename T, int precision>
static inline std::string precise_numstr(T num) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << num;
    return oss.str();
}

void show_all_envelope_points();
void show_all_midi_items();
void show_selected_midi_items();
void show_thing_under_point();
void show_track_ui();
void test();

}
