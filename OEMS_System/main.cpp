#include <conio.h>
#include <chrono>
#include <csignal>
#include <iostream>
#include <limits>
#include <drogon/drogon.h>

#include "order_execution.h"
#include "utilities.h"
#include "web_socket_client.h"

void displayMenu() {
    std::cout << "\n=== Trading System Menu ===\n";
    std::cout << "1. Get Order Book\n";
    std::cout << "2. Place Buy Order\n";
    std::cout << "3. Place Sell Order\n";
    std::cout << "4. Get Current Positions\n";
    std::cout << "5. Exit\n";
    std::cout << "Enter your choice (1-5): ";
}

int main()
{
    std::signal(SIGINT, Utilities::HandleExitSignal);
    std::ios_base::sync_with_stdio(false);

    try
    {
        // Initialize managers
        TokenManager token_manager("access_token.txt", "refresh_token.txt", 2505599);
        const OrderExecution order_execution(token_manager);
        std::string response;
        
        while (true) {
            displayMenu();
            int choice;
            std::cin >> choice;
            std::cin.ignore(100, '\n');

            switch (choice) {
                case 1: {
                    std::cout << "Enter instrument name (e.g., ETH-PERPETUAL): ";
                    std::string instrument;
                    std::getline(std::cin, instrument);
                    
                    if (order_execution.GetOrderBook(instrument, response)) {
                        std::cout << "\nOrder Book for " << instrument << ":\n";
                        std::cout << response << "\n";
                    } else {
                        std::cout << "Failed to get order book.\n";
                    }
                    break;
                }
                case 2: {
                    std::cout << "Enter instrument name: ";
                    std::string instrument;
                    std::getline(std::cin, instrument);

                    double amount, price;
                    std::cout << "Enter amount: ";
                    std::cin >> amount;
                    std::cout << "Enter price: ";
                    std::cin >> price;

                    const OrderParams params{instrument, amount, price, "market" + std::to_string(std::time(nullptr)), OrderType::LIMIT};
                    if (order_execution.PlaceOrder(params, "buy", response)) {
                        std::cout << "Buy order placed successfully:\n" << response << "\n";
                    }
                    break;
                }
                case 3: {
                    std::cout << "Enter instrument name: ";
                    std::string instrument;
                    std::getline(std::cin, instrument);

                    double amount, price;
                    std::cout << "Enter amount: ";
                    std::cin >> amount;
                    std::cout << "Enter price: ";
                    std::cin >> price;

                    const OrderParams params{instrument, amount, price, "market" + std::to_string(std::time(nullptr)), OrderType::LIMIT};
                    if (order_execution.PlaceOrder(params, "sell", response)) {
                        std::cout << "Sell order placed successfully:\n" << response << "\n";
                    }
                    break;
                }
                case 4: {
                    std::cout << "Enter currency (e.g., ETH): ";
                    std::string currency;
                    std::getline(std::cin, currency);

                    if (order_execution.GetCurrentPositions(currency, "future", response)) {
                        std::cout << "\nCurrent Positions:\n";
                        std::cout << response << "\n";
                    }
                    break;
                }
                case 5: {
                    std::cout << "Exiting program...\n";
                    return 0;
                }
                default: {
                    std::cout << "Invalid choice. Please try again.\n";
                    break;
                }
            }

            std::cout << "\nPress Enter to continue...";
            std::cin.get();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}