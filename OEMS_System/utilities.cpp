#include "utilities.h"
#include <drogon/HttpAppFramework.h>

void Utilities::HandleExitSignal(const int signal)
{
    std::cout << "\n[System] Exit signal received: " << signal << ". Shutting down Drogon...\n";
    drogon::app().quit();
}

void Utilities::DisplayJsonResponse(const std::string& response)
{
    Json::Value json_data;
    if (!IsParseJsonGood(response, json_data))
    {
        return;
    }

    if (json_data.isMember("result"))
    {
        const Json::Value& result = json_data["result"];

        if (result.isObject() && result.isMember("order"))
        {
            const Json::Value& order = result["order"];
            std::cout << "\n[Order Details]"
                      << "\nOrder ID: " << order["order_id"].asString()
                      << "\nInstrument: " << order["instrument_name"].asString()
                      << "\nType: " << order["order_type"].asString()
                      << "\nState: " << order["order_state"].asString()
                      << "\nDirection: " << order["direction"].asString()
                      << "\nAmount: " << order["amount"].asDouble()
                      << "\nPrice: " << order["price"].asDouble()
                      << "\nTime in Force: " << order["time_in_force"].asString()
                      << "\nCreation Time: " << DisplayFormattedTimestamp(order["creation_timestamp"].asInt64())
                      << "\n\n";
        }
        else if (result.isObject() && result.isMember("order_id"))
        {
            std::cout << "\n[Cancel Confirmation] Order ID: " << result["order_id"].asString() << " cancelled successfully\n\n";
        }
        else if (result.isArray())
        {
            std::cout << "\n[Open Orders Summary] Total Count: " << result.size() << "\n";
            std::cout << "----------------------------------------\n";

            for (const auto& order : result)
            {
                std::cout << "[Order]"
                          << "\nID: " << order["order_id"].asString()
                          << "\nInstrument: " << order["instrument_name"].asString()
                          << "\nType: " << order["order_type"].asString()
                          << "\nState: " << order["order_state"].asString()
                          << "\nDirection: " << order["direction"].asString()
                          << "\nAmount: " << order["amount"].asDouble()
                          << "\nFilled: " << order["filled_amount"].asDouble()
                          << "\nPrice: " << order["price"].asDouble()
                          << "\nTime in Force: " << order["time_in_force"].asString()
                          << "\nCreation Time: " << DisplayFormattedTimestamp(order["creation_timestamp"].asInt64())
                          << "\n----------------------------------------\n";
            }
        }
        else
        {
            std::cout << "\n[Warning] Unhandled JSON structure in result\n";
        }
    }
    else
    {
        std::cerr << "\n[Error] Unexpected JSON structure: 'result' field not found\n";
    }
}

std::string Utilities::DisplayFormattedTimestamp(const int64_t& timestamp_ms)
{
    std::stringstream time;
    const std::time_t timestamp_sec = timestamp_ms / 1000;
    std::tm tm_time;
    if (gmtime_s(&tm_time, &timestamp_sec) != 0)
    {
        return "[Error] Invalid timestamp";
    }
    time << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S UTC");
    return time.str();
}

void Utilities::DisplayCurrentPositionsJson(const std::string& response)
{
    Json::Value json_data;
    if (!IsParseJsonGood(response, json_data))
    {
        return;
    }

    if (json_data.isMember("result") && json_data["result"].isArray())
    {
        const Json::Value& positions = json_data["result"];
        std::cout << "\n[Current Positions Summary] Total Count: " << positions.size() << "\n";
        std::cout << "============================================\n";
        
        for (const auto& position : positions)
        {
            std::cout << "[Position Details]"
                      << "\nInstrument: " << position["instrument_name"].asString()
                      << "\nDirection: " << position["direction"].asString()
                      << "\nSize: " << position["size"].asDouble()
                      << "\nMark Price: " << position["mark_price"].asDouble()
                      << "\nAverage Price: " << position["average_price"].asDouble()
                      << "\nFloating P&L: " << position["floating_profit_loss"].asDouble()
                      << "\nTotal P&L: " << position["total_profit_loss"].asDouble()
                      << "\nLeverage: " << position["leverage"].asDouble()
                      << "\nMaintenance Margin: " << position["maintenance_margin"].asDouble()
                      << "\nInitial Margin: " << position["initial_margin"].asDouble()
                      << "\nOpen Orders Margin: " << position["open_orders_margin"].asDouble()
                      << "\nTimestamp: " << DisplayFormattedTimestamp(position["creation_timestamp"].asInt64())
                      << "\n============================================\n";
        }
    }
    else
    {
        std::cerr << "\n[Error] Invalid position data structure\n";
    }
}

void Utilities::DisplayOrderBookJson(const std::string& response)
{
    Json::Value json_data;
    if (!IsParseJsonGood(response, json_data))
    {
        return;
    }

    if (json_data.isMember("result"))
    {
        const Json::Value& result = json_data["result"];
        std::cout << "\n[Order Book Summary]"
                  << "\nInstrument: " << result["instrument_name"].asString()
                  << "\nBest Bid: " << result["best_bid_price"].asDouble()
                  << "\nBest Ask: " << result["best_ask_price"].asDouble()
                  << "\nMark Price: " << result["mark_price"].asDouble()
                  << "\nIndex Price: " << result["index_price"].asDouble()
                  << "\n";

        if (result.isMember("bids") && result["bids"].isArray())
        {
            std::cout << "\n[Bids]";
            for (const auto& bid : result["bids"])
            {
                std::cout << "\nPrice: " << bid[0].asDouble() 
                          << " | Amount: " << bid[1].asDouble();
            }
        }

        if (result.isMember("asks") && result["asks"].isArray())
        {
            std::cout << "\n\n[Asks]";
            for (const auto& ask : result["asks"])
            {
                std::cout << "\nPrice: " << ask[0].asDouble() 
                          << " | Amount: " << ask[1].asDouble();
            }
        }
        std::cout << "\n";
    }
    else
    {
        std::cerr << "\n[Error] Invalid order book data structure\n";
    }
    std::cout << "\n";
}

bool Utilities::IsParseJsonGood(const std::string& response, Json::Value& json_data)
{
    const Json::CharReaderBuilder reader_builder;
    std::string errs;

    std::istringstream s(response);
    if (!Json::parseFromStream(reader_builder, s, &json_data, &errs))
    {
        std::cerr << "[Error] JSON Parsing Failed: " << errs << "\n";
        return false;
    }

    // Check if there's an error in the response
    if (json_data.isMember("error"))
    {
        std::cerr << "[API Error] " 
                  << json_data["error"]["message"].asString() << " (Code: " 
                  << json_data["error"]["code"].asInt() << ")\n";
        return false;
    }

    return true;
}