#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace band3 {

struct Config {
    long controller_type = 7;
    long sync = -1;
    std::string forced_venue = "false";
    bool disable_metamusic = false;
};

const Config& GetConfig();
void LoadConfig(const char* path = "band3_config.ini");

const std::vector<std::string>& GetArgs();

}
