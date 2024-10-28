# GoQuant Order Execution and Management System (OEMS)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Dependencies](#dependencies)
- [Installation](#installation)

## Overview

GoQuant OEMS is a high-performance C++17 application designed for interacting with the Deribit cryptocurrency trading platform. This application supports various trading functionalities, including placing, modifying, and canceling orders, as well as managing open orders, positions, and retrieving order books. It includes a WebSocket server to subscribe clients to specific symbols and continuously stream order book updates.

## Features

- **Place Orders:** Place market and limit orders on Deribit.
- **Modify Orders:** Update existing orders with new quantities or prices.
- **Cancel Orders:** Cancel open orders by order ID.
- **Retrieve Order Book:** Fetch and display the order book for specific trading pairs.
- **View Current Positions:** Display current open positions.
- **WebSocket Server:** Allows clients to subscribe to symbols and receive real-time order book updates.
- **Supported Markets:** Spot, futures, and options for all supported symbols.

## Prerequisites

- C++ Compiler (C++17 or later)
- CMake (version 3.10 or later)
- Visual Studio 2022 or another compatible C++ IDE
- vcpkg package manager

## Dependencies

- [Drogon](https://github.com/drogonframework/drogon): For HTTP and WebSocket client/server functionality
- [cURL](https://curl.se/): For HTTP requests
- [OpenSSL](https://www.openssl.org/): For handling secure connections
- [JsonCpp](https://github.com/open-source-parsers/jsoncpp): For JSON parsing

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/GoQuantOEMS.git
   cd GoQuantOEMS
