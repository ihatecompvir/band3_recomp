#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace band3 {

struct Config {
    long controller_type = 7;
    long sync = -1;
    std::string forced_venue = "false";
    bool fullscreen = false;
    long width = 1280;
    long height = 720;
    bool fast_start = false;
    std::string lang;
    bool disable_metamusic = false;
    bool disable_approximate_lights = true;
    bool disable_hair_shader = false;
    bool fullbright = false;
    bool compress_character_textures = false;
    long main_heap_size = 0;
    long char_heap_size = 0;
    std::string username;
    long max_queued_frames = 3;
    bool debug_overlay = true;
    std::string log_level = "info";
};

const Config& GetConfig();
void LoadConfig(const char* path = "band3_config.ini");

const std::vector<std::string>& GetArgs();

}
