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
    g_config.forced_venue =
        reader.Get("venue", "forced_venue", g_config.forced_venue);
    g_config.fullscreen =
        reader.GetBoolean("window", "fullscreen", g_config.fullscreen);
    g_config.width =
        reader.GetInteger("window", "width", g_config.width);
    g_config.height =
        reader.GetInteger("window", "height", g_config.height);
    g_config.fast_start =
        reader.GetBoolean("game", "fast_start", g_config.fast_start);
    g_config.disable_metamusic =
        reader.GetBoolean("game", "disable_metamusic", g_config.disable_metamusic);
    g_config.lang =
        reader.Get("game", "lang", g_config.lang);
    g_config.debug_overlay =
        reader.GetBoolean("debug", "overlay", g_config.debug_overlay);
    g_config.log_level =
        reader.Get("debug", "log_level", g_config.log_level);

    // Inject config-driven args into the command line
    GetArgs();
    if (g_config.fast_start) {
        g_args.push_back("-fast");
    }
    if (!g_config.lang.empty()) {
        g_args.push_back("-lang");
        g_args.push_back(g_config.lang);
    }

    // hard defines for various usecases
    // Rock Band 3 DX identifier
    g_args.push_back("-define");
    g_args.push_back("MHX_PC");
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
		// quality code alert
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
