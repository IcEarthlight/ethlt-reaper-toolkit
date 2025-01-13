#include "my_plugin.h"
#include "reaper_vararg.hpp"
#include <gsl/gsl>

#define STRINGIZE_DEF(x) #x
#define STRINGIZE(x) STRINGIZE_DEF(x)
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
// register main function on timer
// true or false
#define API_ID MYAPI

// confine my plugin to namespace
namespace PROJECT_NAME
{

struct ActionInfo {
    int command_id;
    bool toggle_state;
    const char* command_name;
    const char* action_name;
    bool run_on_timer;  // Individual timer setting for each action
    custom_action_register_t action;
    std::function<void()> onaction; // Function to call when action triggered
};

// Define your actions here with individual timer settings
std::vector<ActionInfo> actions = {
    {0, false, "ETHLT_SMART_VOLUP_COMMAND",   "ethlt: Smart Volume up",   false, {0}, []() {
        // Smart volume up implementation
        ShowConsoleMsg("Smart volume up\n");
    }},
    {0, false, "ETHLT_SMART_VOLDOWN_COMMAND", "ethlt: Smart Volume down", false, {0}, []() {
        // Smart volume down implementation
        ShowConsoleMsg("Smart volume down\n");
    }}
};

// hInstance is declared in header file my_plugin.hpp
// defined here
REAPER_PLUGIN_HINSTANCE hInstance{nullptr}; // used for dialogs, if any

// REAPER calls this to check my plugin toggle state
int ToggleActionCallback(int command)
{
    for (ActionInfo& action_info : actions) {
        if (command == action_info.command_id) {
            if (action_info.toggle_state)
                return 1;
            return 0;
        }
    }
    // not quite our command_id
    return -1;
}

// this gets called when my plugin action is run (e.g. from action list)
bool OnAction(KbdSectionInfo* sec, int command, int val, int valhw, int relmode, HWND hwnd)
{
    // treat unused variables 'pedantically'
    (void)sec; (void)val; (void)valhw; (void)relmode; (void)hwnd;

    for (ActionInfo& action_info : actions) {
        // check command
        if (command != action_info.command_id)
            continue;
        
        // register action-specific function to timer
        if (action_info.run_on_timer) {
            action_info.toggle_state = !action_info.toggle_state; // flip state on/off

            if (action_info.toggle_state) {
                plugin_register("timer", (void*)action_info.onaction.target<void()>()); // "reaper.defer(action)"
            } else {
                plugin_register("-timer", (void*)action_info.onaction.target<void()>()); // "reaper.atexit(shutdown)" 
            }
        } else {
            action_info.onaction(); // Call the action-specific function
        }
        return true;
    }
    return false;
}

// definition string for example API function
const char *reascript_api_function_example_defstring =
    "int" // return type
    "\0"  // delimiter ('separator')
    // input parameter types
    "int,bool,double,const char*,int,const int*,double*,char*,int"
    "\0"
    // input parameter names
    "whole_number,boolean_value,decimal_number,string_of_text,"
    "string_of_text_sz,input_parameterInOptional,"
    "return_valueOutOptional,"
    "return_stringOutOptional,return_stringOutsz"
    "\0"
    "help text for myfunction\n"
    "If optional input parameter is provided, produces optional return "
    "value.\n"
    "If boolean is true, copies input string to optional output string.\n";

// example api function
int ReaScriptAPIFunctionExample(
    int whole_number,
    bool boolean_value,
    double decimal_number,
    const char* string_of_text,
    int string_of_text_sz,
    const int* input_parameterInOptional,
    double* return_valueOutOptional,
    char* return_stringOutOptional,
    int return_string_sz
)
{
    // if optional integer is provided
    if (input_parameterInOptional != nullptr)
    {
        // assign value to deferenced output pointer
        *return_valueOutOptional =
            // by making this awesome calculation
            (*input_parameterInOptional + whole_number + decimal_number);
    }

    // lets conditionally produce optional output string
    if (boolean_value)
    {
        // copy string_of_text to return_stringOutOptional
        // *_sz is length/size of zero terminated string (C-style char array)
        memcpy(return_stringOutOptional, string_of_text, min(return_string_sz, string_of_text_sz) * sizeof(char));
    }
    return whole_number * whole_number;
}

const char *defstring_GetVersion =
    "void" // return type
    "\0"   // delimiter ('separator')
    // input parameter types
    "int*,int*,int*,int*,char*,int"
    "\0"
    // input parameter names
    "majorOut,minorOut,patchOut,tweakOut,commitOut,commitOut_sz"
    "\0"
    "returns version numbers of my plugin\n";

void GetVersion(int* majorOut, int* minorOut, int* patchOut, int* tweakOut, char* commitOut, int commitOut_sz)
{
    *majorOut = PROJECT_VERSION_MAJOR;
    *minorOut = PROJECT_VERSION_MINOR;
    *patchOut = PROJECT_VERSION_PATCH;
    *tweakOut = PROJECT_VERSION_TWEAK;
    const char* commit = STRINGIZE(PROJECT_VERSION_COMMIT);
    std::copy(commit, commit + min(commitOut_sz - 1, (int)strlen(commit)), commitOut);
    commitOut[min(commitOut_sz - 1, (int)strlen(commit))] = '\0'; // Ensure null termination
}

// when my plugin gets loaded
// function to register my plugins 'stuff' with REAPER
void Register()
{
    bool anyone_run_on_timer{false};
    // register each action
    for (ActionInfo& action_info : actions) {
        // register action name and get command_id
        action_info.action = {0, action_info.command_name, action_info.action_name, nullptr};
        action_info.command_id = plugin_register("custom_action", &action_info.action);
        
        // register action on/off state and callback function
        if (action_info.run_on_timer) {
            anyone_run_on_timer = true;
        }
    }
    if (anyone_run_on_timer) {
        plugin_register("toggleaction", (void*)ToggleActionCallback);
    }

    // register run action/command
    plugin_register("hookcommand2", (void*)OnAction);

    // register the API function example
    // function, definition string and function 'signature'
    plugin_register("API_" STRINGIZE(API_ID)"_ReaScriptAPIFunctionExample", (void*)ReaScriptAPIFunctionExample);
    plugin_register(
        "APIdef_" STRINGIZE(API_ID)"_ReaScriptAPIFunctionExample", (void*)reascript_api_function_example_defstring
    );
    plugin_register("APIvararg_" STRINGIZE(API_ID)"_ReaScriptAPIFunctionExample", (void*)&InvokeReaScriptAPI<&ReaScriptAPIFunctionExample>);

    plugin_register("API_" STRINGIZE(API_ID)"_GetVersion", (void*)GetVersion);
    plugin_register("APIdef_" STRINGIZE(API_ID)"_GetVersion", (void*)defstring_GetVersion);
    plugin_register("APIvararg_" STRINGIZE(API_ID)"_GetVersion", (void*)&InvokeReaScriptAPI<&GetVersion>);
}

// shutdown, time to exit
void Unregister()
{
    // unregister each action
    for (ActionInfo& action_info : actions) {
        plugin_register("-custom_action", &action_info.action);
    }
    plugin_register("-toggleaction", (void*)ToggleActionCallback);
    plugin_register("-hookcommand2", (void*)OnAction);
}

} // namespace PROJECT_NAME