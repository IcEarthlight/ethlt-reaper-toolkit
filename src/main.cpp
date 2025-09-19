#define REAPERAPI_IMPLEMENT
#include "ethlt_reaper_toolkit.h"
#include <WDL/wdltypes.h> // might be unnecessary in future
#include <reaper_plugin_functions.h>

extern "C"
{

REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance,
                                                      reaper_plugin_info_t *rec)
{

    PROJECT_NAME::hInstance = hInstance;
    if (rec != nullptr && REAPERAPI_LoadAPI(rec->GetFunc) == 0) {
        // check that our plugin hasn't been already loaded
        if (rec->GetFunc("ReaScriptAPIFunctionExample")) return 0;
        PROJECT_NAME::Register();
        return 1;
    }
    // quit
    PROJECT_NAME::Unregister();
    return 0;
}
}
