cmake_minimum_required(VERSION 3.14)
project(OEMS_System)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Find required packages
find_package(Drogon CONFIG REQUIRED)
find_package(websocketpp CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

# Add source files
add_executable(${PROJECT_NAME}
    main.cpp
    api_credentials.cpp
    order_execution.cpp
    token_manager.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    websocketpp::websocketpp
    TBB::tbb
)