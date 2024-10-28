#pragma once
#include <string>

#include <drogon/drogon.h>

class Utilities
{
  public:
    static void HandleExitSignal(const int signal);
    static void DisplayJsonResponse(const std::string& response);

    static std::string DisplayFormattedTimestamp(const int64_t& timestamp_ms);

    static void DisplayCurrentPositionsJson(const std::string& response);
    static bool IsParseJsonGood(const std::string& response, Json::Value& json_data);

    static void DisplayOrderBookJson(const std::string& response);
};
