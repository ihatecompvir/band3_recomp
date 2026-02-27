#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace band3 {

struct Config {
    long controller_type = 7;
    long sync = -1;
    bool fullscreen = false;
    long width = 1280;
    long height = 720;
    bool fast_start = false;
    bool debug_overlay = true;
    std::string log_level = "info";
};

const Config& GetConfig();
void LoadConfig(const char* path = "band3_config.ini");

const std::vector<std::string>& GetArgs();

}
