#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

static std::string trim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
    return s;
}

bool loadConfig(const std::string& path, ServiceConfig& cfg, std::string& err) {
    std::ifstream in(path);
    if (!in) { err = "Cannot open config file"; return false; }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        if (key == "gpio_pin") cfg.gpio_pin = std::stoi(value);
        else if (key == "interval_seconds") cfg.interval_seconds = std::stoi(value);
        else if (key == "api_base") cfg.api_base = value;
        else if (key == "api_key") cfg.api_key = value;
        else if (key == "source_id") cfg.source_id = value;
    }
    if (cfg.api_base.empty() || cfg.api_key.empty() || cfg.source_id.empty()) {
        err = "Missing required api_base/api_key/source_id";
        return false;
    }
    return true;
}
