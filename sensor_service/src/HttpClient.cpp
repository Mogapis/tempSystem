#include "HttpClient.hpp"
#include <curl/curl.h>
#include <sstream>
#include <iomanip>
#include <ctime>

static std::string jsonEscape(const std::string& s) {
    std::ostringstream o;
    for (auto c: s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            default: o << c;
        }
    }
    return o.str();
}

bool postReading(const std::string& apiBase,
                 const std::string& apiKey,
                 const std::string& sourceId,
                 double tempC,
                 double humidity,
                 const std::string& isoTimestamp,
                 std::string& err) {
    std::string url = apiBase + "/api/ingest";
    CURL* curl = curl_easy_init();
    if (!curl) { err = "curl init failed"; return false; }

    std::ostringstream body;
    body << "{"
         << "\"temperature_c\":" << std::fixed << std::setprecision(2) << tempC << ","
         << "\"humidity_rel\":" << std::fixed << std::setprecision(2) << humidity << ","
         << "\"timestamp_utc\":\"" << jsonEscape(isoTimestamp) << "\","
         << "\"source\":\"" << jsonEscape(sourceId) << "\""
         << "}";

    struct curl_slist* headers = nullptr;
    std::string apiKeyHdr = "X-API-Key: " + apiKey;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, apiKeyHdr.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    auto bodyStr = body.str();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    long code = 0;
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if (res != CURLE_OK || code < 200 || code >= 300) {
        std::ostringstream e;
        e << "HTTP failure: curl=" << curl_easy_strerror(res) << " code=" << code;
        err = e.str();
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return true;
}