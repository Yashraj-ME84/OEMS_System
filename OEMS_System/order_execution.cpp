#include "order_execution.h"
#include <cstdio>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <drogon/drogon.h>
#include "utilities.h"

class RateLimiter {
private:
    std::chrono::steady_clock::time_point lastRequest;
    const std::chrono::milliseconds minInterval{100}; // 100ms between requests

public:
    void WaitIfNeeded() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - lastRequest;
        if (elapsed < minInterval) {
            std::this_thread::sleep_for(minInterval - elapsed);
        }
        lastRequest = std::chrono::steady_clock::now();
    }
};

OrderExecution::OrderExecution(TokenManager& token_manager)
    : m_client(drogon::HttpClient::newHttpClient(BASE_URL)),
      m_token_manager(token_manager),
      m_api_credentials("client_key.txt", "client_secret.txt"),
      m_rate_limiter(std::make_unique<RateLimiter>())
{
}

bool OrderExecution::RefreshTokenIfNeeded() const
{
    if (m_token_manager.IsAccessTokenExpired())
    {
        std::cerr << "Access token expired. Refreshing...\n";
        if (!m_token_manager.RefreshAccessToken(m_api_credentials.GetApiKey(),
                                                m_api_credentials.GetApiSecret()))
        {
            std::cerr << "Failed to refresh token. Cannot place order.\n";
            return false;
        }
    }
    return true;
}

std::string OrderExecution::GetOrderTypeString(const OrderType& type)
{
    switch (type)
    {
        case OrderType::LIMIT: return "limit";
        case OrderType::MARKET: return "market";
        case OrderType::STOP_LIMIT: return "stop_limit";
        case OrderType::STOP_MARKET: return "stop_market";
        default: return "limit";
    }
}

struct ApiResponse {
    bool success;
    std::string message;
    std::string data;
};

ApiResponse HandleResponse(const drogon::ReqResult& result, const drogon::HttpResponsePtr& response) {
    if (result != drogon::ReqResult::Ok) {
        return {false, "Network error", ""};
    }
    if (!response) {
        return {false, "Empty response", ""};
    }
    if (response->getStatusCode() != drogon::k200OK) {
        return {false, "HTTP error: " + std::to_string(response->getStatusCode()), std::string(response->body())};
    }
    return {true, "Success", std::string(response->body())};
}

bool OrderExecution::ValidateOrderParams(const OrderParams& params) const {
    if (params.amount <= 0) {
        std::cerr << "Invalid amount: " << params.amount << std::endl;
        return false;
    }
    if (params.type == OrderType::LIMIT && params.price <= 0) {
        std::cerr << "Invalid price for limit order: " << params.price << std::endl;
        return false;
    }
    return true;
}

template<typename Callback>
void OrderExecution::SendAsyncRequest(const drogon::HttpRequestPtr& req, Callback&& callback) const {
    m_rate_limiter->WaitIfNeeded();
    m_client->sendRequest(req, std::forward<Callback>(callback));
}

template<typename Func>
bool OrderExecution::RetryRequest(Func&& func, int maxRetries = 3) const {
    for (int i = 0; i < maxRetries; i++) {
        if (func()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * (i + 1)));
    }
    return false;
}

bool OrderExecution::PlaceOrder(const OrderParams& params, const std::string& side, std::string& response) const
{
    if (!ValidateOrderParams(params) || !RefreshTokenIfNeeded()) {
        return false;
    }

    const std::string access_token = m_token_manager.GetAccessToken();
    const auto req = drogon::HttpRequest::newHttpRequest();

    req->setPath(side == "buy" ? "/api/v2/private/buy" : "/api/v2/private/sell");
    req->setMethod(drogon::Get);

    char buffer[BUFFER_SIZE];
    int written;

    if (params.type == OrderType::LIMIT)
    {
        written = snprintf(buffer, BUFFER_SIZE, "%s?amount=%.6f&instrument_name=%s&label=%s&price=%.2f&type=%s",
                           req->getPath().c_str(), params.amount, params.instrument_name.c_str(),
                           params.label.c_str(), params.price, GetOrderTypeString(params.type).c_str());
    }
    else if (params.type == OrderType::MARKET)
    {
        written = snprintf(buffer, BUFFER_SIZE, "%s?amount=%.6f&instrument_name=%s&label=%s&type=%s",
                           req->getPath().c_str(), params.amount, params.instrument_name.c_str(),
                           params.label.c_str(), GetOrderTypeString(params.type).c_str());
    }
    else
    {
        std::cerr << "Unsupported order type.\n";
        return false;
    }

    if (written < 0 || written >= BUFFER_SIZE)
    {
        std::cerr << "Buffer overflow in request formatting\n";
        return false;
    }

    req->setPath(std::string(buffer, written));
    req->addHeader("Authorization", "Bearer " + access_token);
    req->addHeader("Content-Type", "application/x-www-form-urlencoded");

    std::promise<bool> requestComplete;
    auto future = requestComplete.get_future();

    SendAsyncRequest(req, 
        [&response, &requestComplete](const drogon::ReqResult& result, const drogon::HttpResponsePtr& http_response) {
            auto apiResponse = HandleResponse(result, http_response);
            if (apiResponse.success) {
                response = apiResponse.data;
                std::cout << "Placed Order:\n";
                Utilities::DisplayJsonResponse(response);
                requestComplete.set_value(true);
            } else {
                std::cerr << "Error: " << apiResponse.message << std::endl;
                response = apiResponse.message;
                requestComplete.set_value(false);
            }
        });

    return RetryRequest([&future]() { 
        return future.wait_for(std::chrono::seconds(5)) == std::future_status::ready && future.get(); 
    });
}

// Similar changes should be applied to CancelOrder, ModifyOrder, GetOrderBook, GetCurrentPositions, and GetOpenOrders methods.

bool OrderExecution::CancelOrder(const std::string& order_id, std::string& response) const
{
    if (!RefreshTokenIfNeeded())
    {
        return false;
    }

    const auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);

    char buffer[BUFFER_SIZE];
    const int written = snprintf(buffer, BUFFER_SIZE, "/api/v2/private/cancel?order_id=%s", order_id.c_str());

    if (written < 0 || written >= BUFFER_SIZE)
    {
        std::cerr << "Buffer overflow or error in sprintf.\n";
        return false;
    }

    req->setPath(std::string(buffer, written));
    req->addHeader("Authorization", "Bearer " + m_token_manager.GetAccessToken());
    req->addHeader("Content-Type", "application/json");

    std::promise<bool> requestComplete;
    auto future = requestComplete.get_future();

    SendAsyncRequest(req, 
        [&response, &requestComplete](const drogon::ReqResult& result, const drogon::HttpResponsePtr& http_response) {
            auto apiResponse = HandleResponse(result, http_response);
            if (apiResponse.success) {
                response = apiResponse.data;
                Utilities::DisplayJsonResponse(response);
                requestComplete.set_value(true);
            } else {
                std::cerr << "Error canceling order: " << apiResponse.message << std::endl;
                response = apiResponse.message;
                requestComplete.set_value(false);
            }
        });

    return RetryRequest([&future]() { 
        return future.wait_for(std::chrono::seconds(5)) == std::future_status::ready && future.get(); 
    });
}

bool OrderExecution::ModifyOrder(const std::string& order_id, const double& new_amount, const double& new_price,
                             std::string& response) const
{
    if (!RefreshTokenIfNeeded() || new_amount <= 0 || new_price <= 0)
    {
        return false;
    }

    const auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);

    char buffer[BUFFER_SIZE];
    const int written = snprintf(buffer, BUFFER_SIZE, 
        "/api/v2/private/edit?order_id=%s&amount=%.6f&price=%.2f",
        order_id.c_str(), new_amount, new_price);

    if (written < 0 || written >= BUFFER_SIZE)
    {
        std::cerr << "Buffer overflow or error in sprintf.\n";
        return false;
    }

    req->setPath(std::string(buffer, written));
    req->addHeader("Authorization", "Bearer " + m_token_manager.GetAccessToken());
    req->addHeader("Content-Type", "application/json");

    std::promise<bool> requestComplete;
    auto future = requestComplete.get_future();

    SendAsyncRequest(req, 
        [&response, &requestComplete](const drogon::ReqResult& result, const drogon::HttpResponsePtr& http_response) {
            auto apiResponse = HandleResponse(result, http_response);
            if (apiResponse.success) {
                response = apiResponse.data;
                std::cout << "Modified Order:\n";
                Utilities::DisplayJsonResponse(response);
                requestComplete.set_value(true);
            } else {
                std::cerr << "Error modifying order: " << apiResponse.message << std::endl;
                response = apiResponse.message;
                requestComplete.set_value(false);
            }
        });

    return RetryRequest([&future]() { 
        return future.wait_for(std::chrono::seconds(5)) == std::future_status::ready && future.get(); 
    });
}

bool OrderExecution::GetOrderBook(const std::string& instrument_name, std::string& response) const
{
    if (instrument_name.empty()) {
        std::cerr << "Invalid instrument name\n";
        return false;
    }

    const auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/v2/public/get_order_book?instrument_name=" + instrument_name);

    std::promise<bool> requestComplete;
    auto future = requestComplete.get_future();

    SendAsyncRequest(req, 
        [&response, &requestComplete](const drogon::ReqResult& result, const drogon::HttpResponsePtr& http_response) {
            auto apiResponse = HandleResponse(result, http_response);
            if (apiResponse.success) {
                response = apiResponse.data;
                Utilities::DisplayOrderBookJson(response);
                requestComplete.set_value(true);
            } else {
                std::cerr << "Error getting order book: " << apiResponse.message << std::endl;
                response = apiResponse.message;
                requestComplete.set_value(false);
            }
        });

    return RetryRequest([&future]() { 
        return future.wait_for(std::chrono::seconds(5)) == std::future_status::ready && future.get(); 
    });
}

bool OrderExecution::GetCurrentPositions(const std::string& currency, const std::string& kind,
                                     std::string& response) const
{
    if (!RefreshTokenIfNeeded() || currency.empty())
    {
        return false;
    }

    const auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);

    std::string path = "/api/v2/private/get_positions?currency=" + currency;
    if (!kind.empty())
    {
        path += "&kind=" + kind;
    }
    req->setPath(path);
    req->addHeader("Authorization", "Bearer " + m_token_manager.GetAccessToken());
    req->addHeader("Content-Type", "application/json");

    std::promise<bool> requestComplete;
    auto future = requestComplete.get_future();

    SendAsyncRequest(req, 
        [&response, &requestComplete](const drogon::ReqResult& result, const drogon::HttpResponsePtr& http_response) {
            auto apiResponse = HandleResponse(result, http_response);
            if (apiResponse.success) {
                response = apiResponse.data;
                Utilities::DisplayCurrentPositionsJson(response);
                requestComplete.set_value(true);
            } else {
                std::cerr << "Error getting positions: " << apiResponse.message << std::endl;
                response = apiResponse.message;
                requestComplete.set_value(false);
            }
        });

    return RetryRequest([&future]() { 
        return future.wait_for(std::chrono::seconds(5)) == std::future_status::ready && future.get(); 
    });
}

bool OrderExecution::GetOpenOrders(std::string& response) const
{
    if (!RefreshTokenIfNeeded())
    {
        return false;
    }

    const auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/v2/private/get_open_orders");
    req->addHeader("Authorization", "Bearer " + m_token_manager.GetAccessToken());
    req->addHeader("Content-Type", "application/json");

    std::promise<bool> requestComplete;
    auto future = requestComplete.get_future();

    SendAsyncRequest(req, 
        [this, &response, &requestComplete](const drogon::ReqResult& result, const drogon::HttpResponsePtr& http_response) {
            auto apiResponse = ProcessHttpResponse(result, http_response);
            if (apiResponse.success) {
                response = apiResponse.data;
                std::cout << "Open Orders:\n";
                Utilities::DisplayJsonResponse(response);
                requestComplete.set_value(true);
            } else {
                std::cerr << "Error getting open orders: " << apiResponse.message << std::endl;
                response = "Failed to get open orders: " + apiResponse.message;
                requestComplete.set_value(false);
            }
        });

    return RetryRequest([&future]() { 
        return future.wait_for(std::chrono::seconds(5)) == std::future_status::ready && future.get(); 
    });
}