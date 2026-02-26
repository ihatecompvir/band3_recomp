#include "config.h"
#include "ThirdParty/inih/INIReader.h"
#include <rex/logging.h>

#ifdef _WIN32
#include <cstdlib>
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace band3 {

static Config g_config;
static std::vector<std::string> g_args;
static bool g_args_initialized = false;

const Config& GetConfig() { return g_config; }

void LoadConfig(const char* path) {
    INIReader reader(path);
    if (reader.ParseError() != 0) {
        REXLOG_WARN("Failed to load {}: {}", path, reader.ParseErrorMessage());
        return;
    }

    g_config.controller_type =
        reader.GetInteger("controller", "type", g_config.controller_type);
    g_config.sync =
        reader.GetInteger("rnd", "sync", g_config.sync);
    g_config.forced_venue =
        reader.Get("venue", "forced_venue", g_config.forced_venue);
    g_config.disable_metamusic =
        reader.GetBoolean("patches", "disable_metamusic", g_config.disable_metamusic);
    REXLOG_INFO("Config: controller_type={}, sync={}, forced_venue={}, disable_metamusic={}",
        g_config.controller_type, g_config.sync, g_config.forced_venue, g_config.disable_metamusic);
}

const std::vector<std::string>& GetArgs() {
    if (!g_args_initialized) {
        g_args_initialized = true;
#ifdef _WIN32
        for (int i = 0; i < __argc; i++) {
            int len = WideCharToMultiByte(CP_UTF8, 0, __wargv[i], -1,
                                          nullptr, 0, nullptr, nullptr);
            std::string s(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, __wargv[i], -1,
                                s.data(), len, nullptr, nullptr);
            g_args.push_back(std::move(s));
        }
#else
        for (int i = 0; i < __argc; i++) {
            g_args.push_back(__argv[i]);
        }
#endif
    }
    return g_args;
}

}
