#include "api_credentials.h"

#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
    // Constants
    constexpr size_t MAX_KEY_LENGTH = 128;
    
    // Helper function to get current timestamp for logging
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_time;
        localtime_s(&tm_time, &time);
        std::ostringstream ss;
        ss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // Helper function for logging
    void LogMessage(const std::string& message, bool is_error = false) {
        std::ostream& output = is_error ? std::cerr : std::cout;
        output << "[" << GetTimestamp() << "] " << message << std::endl;
    }
}

ApiCredentials::ApiCredentials(const std::string& key_file_path, const std::string& secret_file_path)
{
    try {
        LogMessage("Initializing API credentials...");
        
        m_client_key = ReadFile(key_file_path);
        m_client_secret = ReadFile(secret_file_path);
        
        // Basic validation of credentials
        if (m_client_key.empty() || m_client_secret.empty()) {
            throw std::runtime_error("API credentials cannot be empty");
        }
        
        if (m_client_key.length() > MAX_KEY_LENGTH || m_client_secret.length() > MAX_KEY_LENGTH) {
            throw std::runtime_error("API credentials exceed maximum allowed length");
        }

        LogMessage("API credentials initialized successfully");
    }
    catch (const std::exception& e) {
        LogMessage("Failed to initialize API credentials: " + std::string(e.what()), true);
        throw;
    }
}

const std::string& ApiCredentials::GetApiKey() const noexcept
{
    return m_client_key;
}

const std::string& ApiCredentials::GetApiSecret() const noexcept
{
    return m_client_secret;
}

std::string ApiCredentials::ReadFile(const std::string& file_path)
{
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open credential file: " + file_path);
        }

        std::string content;
        content.reserve(MAX_KEY_LENGTH);  // Pre-allocate memory
        
        std::getline(file, content);
        file.close();

        // Trim whitespace
        content.erase(0, content.find_first_not_of(" \t\n\r"));
        content.erase(content.find_last_not_of(" \t\n\r") + 1);

        if (content.empty()) {
            throw std::runtime_error("Credential file is empty: " + file_path);
        }

        LogMessage("Successfully read credentials from: " + file_path);
        return content;
    }
    catch (const std::exception& e) {
        LogMessage("Error reading credential file: " + std::string(e.what()), true);
        throw;
    }
}