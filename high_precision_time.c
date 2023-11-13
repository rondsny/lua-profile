#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef _WIN32
#include <windows.h>

static double get_high_precision_time() {
    FILETIME filetime;
    GetSystemTimeAsFileTime(&filetime);

    ULARGE_INTEGER uli;
    uli.LowPart = filetime.dwLowDateTime;
    uli.HighPart = filetime.dwHighDateTime;

    double time = uli.QuadPart / 10000000.0;
    return time;
}

#elif defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>

static double get_high_precision_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time = tv.tv_sec + tv.tv_usec / 1000000.0;
    return time;
}

#else
// 不支持的平台
static double get_high_precision_time() {
    return -1;
}

#endif

static int lua_get_high_precision_time(lua_State* L) {
    double time = get_high_precision_time();
    lua_pushnumber(L, time);
    return 1;
}

static const struct luaL_Reg high_precision_time_lib[] = {
    {"get_high_precision_time", lua_get_high_precision_time},
    {NULL, NULL}
};

int luaopen_high_precision_time(lua_State* L) {
    luaL_newlib(L, high_precision_time_lib);
    return 1;
}
