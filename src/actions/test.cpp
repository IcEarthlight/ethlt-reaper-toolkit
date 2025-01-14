#include "test.h"
#include <stdio.h>

namespace PROJECT_NAME
{

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
void PrintElementUnderPoint(int x, int y) {
    char buf[1024];
    sprintf(buf, "Cursor position: %d, %d\n", x, y);
    ShowConsoleMsg(buf);
    buf[0] = '\0'; // Clear buf

    char info[256];
    GetMousePosition(&x, &y);
    MediaTrack *track = GetThingFromPoint(x, y, info, sizeof(info));
    if (track) {
        char trackName[256];
        GetTrackName(track, trackName, sizeof(trackName));
        sprintf(buf, "GetThingFromPoint: %s %s\n", trackName, info);
    } else {
        sprintf(buf, "GetThingFromPoint: non-track %s\n", info);
    }
    ShowConsoleMsg(buf[0] == '\0' ? "unknown\n" : buf);
    buf[0] = '\0'; // Clear buf

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
    buf[0] = '\0'; // Clear buf

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
    } else {
        sprintf(buf, "%sTakeInfo: None\n", buf);
    }
    ShowConsoleMsg(buf[0] == '\0' ? "unknown\n" : buf);
    buf[0] = '\0'; // Clear buf
}

void Test()
{
    POINT pt;
    if (!GetCursorPos(&pt)) {
        ShowConsoleMsg("Failed to get cursor position\n");
        return;
    }
    if (active_midi_editor()) {
        ShowConsoleMsg("MIDI Editor is active\n");
    }
    PrintElementUnderPoint(pt.x, pt.y);
}

}