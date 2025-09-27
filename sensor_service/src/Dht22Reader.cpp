#include "Dht22Reader.hpp"
#include <wiringPi.h>
#include <chrono>
#include <thread>

Dht22Reader::Dht22Reader(int gpioPin) : pin(gpioPin) {}

bool Dht22Reader::init(std::string& err) {
    // Use BCM numbering
    if (wiringPiSetupGpio() != 0) {
        err = "wiringPiSetupGpio failed";
        return false;
    }
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    initialised = true;
    return true;
}

uint64_t Dht22Reader::nowMs() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

bool Dht22Reader::performTransaction(uint8_t data[5], std::string& err) {
    // Clear buffer
    for (int i = 0; i < 5; ++i) data[i] = 0;

    // 1. Start signal
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(2); // 2 ms > 1 ms minimum
    digitalWrite(pin, HIGH);
    delayMicroseconds(30);
    pinMode(pin, INPUT);

    // 2. Wait for sensor response: 80us low then 80us high
    auto waitLevel = [&](int desired, int timeoutUs) -> bool {
        int start = micros();
        while (micros() - start < timeoutUs) {
            if (digitalRead(pin) == desired) return true;
        }
        return false;
    };

    if (!waitLevel(LOW, 100)) {
        err = "No initial LOW (sensor not responding)";
        return false;
    }
    if (!waitLevel(HIGH, 100)) {
        err = "No initial HIGH after LOW";
        return false;
    }
    if (!waitLevel(LOW, 100)) {
        err = "No second LOW (data preamble)";
        return false;
    }

    // 3. Read 40 bits
    for (int bit = 0; bit < 40; ++bit) {
        // Wait for pin to go HIGH (start of bit)
        if (!waitLevel(HIGH, 80)) {
            err = "Bit start HIGH timeout";
            return false;
        }
        int startHigh = micros();

        // Wait for pin to go LOW marking end of bit's high pulse
        if (!waitLevel(LOW, 100)) {
            err = "Bit HIGH pulse timeout";
            return false;
        }
        int pulseLen = micros() - startHigh; // high pulse length

        int byteIdx = bit / 8;
        data[byteIdx] <<= 1;

        // Threshold ~50us baseline: typical 0 ~26-30us high; 1 ~70us high
        if (pulseLen > 50) {
            data[byteIdx] |= 1;
        }
    }

    // 4. Verify checksum
    uint8_t sum = (uint8_t)((data[0] + data[1] + data[2] + data[3]) & 0xFF);
    if (sum != data[4]) {
        err = "Checksum mismatch";
        return false;
    }
    return true;
}

DhtReading Dht22Reader::read() {
    DhtReading r;
    r.timestampEpochMs = nowMs();

    if (!initialised) {
        r.error = "Not initialised";
        return r;
    }

    uint8_t raw[5];
    std::string err;
    bool ok = performTransaction(raw, err);
    if (!ok) {
        r.error = err;
        return r;
    }

    // Data format:
    // Humidity: raw[0] raw[1]
    // Temperature: raw[2] raw[3]
    int humRaw = (raw[0] << 8) | raw[1];
    int tempRaw = (raw[2] << 8) | raw[3];

    r.humidity = humRaw / 10.0;

    if (tempRaw & 0x8000) { // negative
        tempRaw &= 0x7FFF;
        r.temperatureC = - (tempRaw / 10.0);
    } else {
        r.temperatureC = tempRaw / 10.0;
    }

    // Basic plausibility
    if (r.humidity < 0 || r.humidity > 100 || r.temperatureC < -60 || r.temperatureC > 90) {
        r.error = "Out of plausible range";
        return r;
    }

    r.valid = true;
    return r;
}