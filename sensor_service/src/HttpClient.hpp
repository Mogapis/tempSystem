#pragma once
#include <string>

bool postReading(const std::string& apiBase,
                 const std::string& apiKey,
                 const std::string& sourceId,
                 double tempC,
                 double humidity,
                 const std::string& isoTimestamp,
                 std::string& err);