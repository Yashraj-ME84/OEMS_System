#pragma once

#include <memory>
#include <string>
#include <future>
#include <chrono>

#include <drogon/HttpClient.h>

#include "api_credentials.h"
#include "token_manager.h"

enum class OrderType
{
    LIMIT,
    MARKET,
    STOP_LIMIT,
    STOP_MARKET
};

enum class InstrumentType
{
    SPOT,
    FUTURES,
    OPTION
};

struct OrderParams
{
    std::string instrument_name;  // e.g., "BTC-PERPETUAL", "BTC-28JUN24"
    double amount;                // Amount in base currency
    double price;                 // Optional for market orders
    std::string label;            // Client order ID
    OrderType type;               // Order type
    std::string time_in_force;    // "good_til_cancelled", "fill_or_kill" "immediate_or_cancel"
};

struct ApiResponse {
    bool success;
    std::string message;
    std::string data;
};

class RateLimiter;

class OrderExecution
{
  private:
    static constexpr size_t BUFFER_SIZE = 2048;
    static constexpr const char* BASE_URL = "https://test.deribit.com";
    static constexpr const char* API_PATH = "/api/v2/private/";
    std::shared_ptr<drogon::HttpClient> m_client;
    TokenManager& m_token_manager;
    ApiCredentials m_api_credentials;
    std::unique_ptr<RateLimiter> m_rate_limiter;

    bool RefreshTokenIfNeeded() const;
    bool ValidateOrderParams(const OrderParams& params) const;
    
    ApiResponse ProcessHttpResponse(const drogon::ReqResult& result, 
                                  const drogon::HttpResponsePtr& response) const;

    template<typename Callback>
    void SendAsyncRequest(const drogon::HttpRequestPtr& req, Callback&& callback) const;
    
    template<typename Func>
    bool RetryRequest(Func&& func, int maxRetries = 3) const;

  public:
    OrderExecution(TokenManager& token_manager);
    ~OrderExecution() = default;

    OrderExecution(const OrderExecution&) = delete;
    OrderExecution& operator=(const OrderExecution&) = delete;
    OrderExecution(OrderExecution&&) = delete;
    OrderExecution& operator=(OrderExecution&&) = delete;

    static std::string GetOrderTypeString(const OrderType& type);

    bool PlaceOrder(const OrderParams& params, const std::string& side, std::string& response) const;
    bool CancelOrder(const std::string& order_id, std::string& response) const;
    bool ModifyOrder(const std::string& order_id, const double& new_amount, const double& new_price,
                     std::string& response) const;
    bool GetOrderBook(const std::string& instrument_name, std::string& response) const;
    bool GetCurrentPositions(const std::string& currency, const std::string& kind,
                             std::string& response) const;
    bool GetOpenOrders(std::string& response) const;
};

// Template implementations
template<typename Callback>
void OrderExecution::SendAsyncRequest(const drogon::HttpRequestPtr& req, Callback&& callback) const 
{
    // Implementation here
    if (m_rate_limiter && !m_rate_limiter->CheckRateLimit()) {
        callback(drogon::ReqResult::BadResponse, nullptr);
        return;
    }
    m_client->sendRequest(req, std::forward<Callback>(callback));
}

template<typename Func>
bool OrderExecution::RetryRequest(Func&& func, int maxRetries) const 
{
    int attempts = 0;
    while (attempts < maxRetries) {
        if (func()) {
            return true;
        }
        ++attempts;
        if (attempts < maxRetries) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << attempts)));
        }
    }
    return false;
}