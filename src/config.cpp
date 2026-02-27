#include "config.h"
#include "ThirdParty/inih/INIReader.h"
#include <rex/logging.h>

#ifdef _WIN32
#include <cstdlib>
#include <windows.h>
#else
#include <cstdlib>
#include <fstream>
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
    g_config.fullscreen =
        reader.GetBoolean("window", "fullscreen", g_config.fullscreen);
    g_config.width =
        reader.GetInteger("window", "width", g_config.width);
    g_config.height =
        reader.GetInteger("window", "height", g_config.height);
    g_config.fast_start =
        reader.GetBoolean("game", "fast_start", g_config.fast_start);
    g_config.debug_overlay =
        reader.GetBoolean("debug", "overlay", g_config.debug_overlay);
    g_config.log_level =
        reader.Get("debug", "log_level", g_config.log_level);
    REXLOG_INFO("Config: controller_type={}, sync={}, fullscreen={}, "
        "width={}, height={}, fast_start={}, debug_overlay={}, log_level={}",
        g_config.controller_type, g_config.sync, g_config.fullscreen,
        g_config.width, g_config.height, g_config.fast_start,
        g_config.debug_overlay, g_config.log_level);

    // Inject config-driven args into the command line
    if (g_config.fast_start) {
        GetArgs(); // ensure g_args is initialized
        g_args.push_back("fast");
    }
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
        std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
        if (cmdline) {
            std::string arg;
            while (std::getline(cmdline, arg, '\0')) {
                g_args.push_back(std::move(arg));
            }
        }
#endif
    }
    return g_args;
}

}
