#include "Dht22Reader.hpp"
#include "Config.hpp"
#include "HttpClient.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>

static std::string isoUtcNow() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream o;
    o << std::put_time(&tm, "%FT%TZ");
    return o.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: sensor_service <config.yaml>\n";
        return 1;
    }
    std::string cfgErr;
    ServiceConfig cfg;
    if (!loadConfig(argv[1], cfg, cfgErr)) {
        std::cerr << "Config error: " << cfgErr << "\n";
        return 1;
    }

    std::cout << "Starting DHT22 service on GPIO " << cfg.gpio_pin
              << " interval=" << cfg.interval_seconds << "s source=" << cfg.source_id << "\n";

    Dht22Reader reader(cfg.gpio_pin);
    std::string initErr;
    if (!reader.init(initErr)) {
        std::cerr << "Init error: " << initErr << "\n";
        return 1;
    }

    int consecutiveFailures = 0;
    while (true) {
        auto reading = reader.read();
        if (!reading.valid) {
            ++consecutiveFailures;
            std::cerr << "[WARN] Read failed (" << consecutiveFailures << "): " << reading.error << "\n";
        } else {
            consecutiveFailures = 0;
            std::string tsIso = isoUtcNow(); // You could also convert reading.timestampEpochMs if you wanted
            std::string postErr;
            if (postReading(cfg.api_base, cfg.api_key, cfg.source_id,
                            reading.temperatureC, reading.humidity, tsIso, postErr)) {
                std::cout << "[OK] " << tsIso
                          << " T=" << reading.temperatureC << "C"
                          << " H=" << reading.humidity << "%\n";
            } else {
                std::cerr << "[ERR] Post failed: " << postErr << "\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(cfg.interval_seconds));
    }
    return 0;
}