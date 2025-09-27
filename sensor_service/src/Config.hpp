#pragma once
#include <string>

struct ServiceConfig {
    int gpio_pin = 4;
    int interval_seconds = 5;
    std::string api_base;
    std::string api_key;
    std::string source_id;
};

bool loadConfig(const std::string& path, ServiceConfig& cfg, std::string& err);