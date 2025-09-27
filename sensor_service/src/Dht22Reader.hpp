#pragma once
#include <cstdint>
#include <optional>
#include <string>

struct DhtReading {
    double temperatureC{};
    double humidity{};
    uint64_t timestampEpochMs{};
    bool valid{false};
    std::string error;
};

class Dht22Reader {
public:
    explicit Dht22Reader(int gpioPin);
    bool init(std::string& err);
    DhtReading read();

private:
    int pin;
    bool initialised{false};
    bool performTransaction(uint8_t data[5], std::string& err);
    uint64_t nowMs() const;
};