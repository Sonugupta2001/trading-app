# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(DeribitTradingSystem VERSION 1.0)

# Set C++ Standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party  # If you have external dependencies in this folder
)

# Define the source files for your project
set(SOURCES
    src/main.cpp
    src/auth/AuthManager.cpp
    src/auth/utils/HttpClient.cpp
    src/orders/OrderManager.cpp
    src/market/MarketDataManager.cpp
    src/market/models/OrderBook.cpp
    src/websocket/WebSocketServer.cpp
    src/websocket/ClientHandler.cpp
)

# Define the header files for the project
set(HEADERS
    include/Logger.h
    include/Config.h
    include/Utils.h
    src/auth/AuthManager.h
    src/auth/utils/HttpClient.h
    src/orders/OrderManager.h
    src/market/MarketDataManager.h
    src/market/models/OrderBook.h
    src/websocket/WebSocketServer.h
    src/websocket/ClientHandler.h
)

# Create the executable from the source files
add_executable(DeribitTradingSystem ${SOURCES} ${HEADERS})

# External libraries (example for WebSocket if using Boost or another library)
# You need to install WebSocket library and link it here
# target_link_libraries(DeribitTradingSystem Boost::Boost websocketpp)

# Optionally, enable optimizations for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
endif()

# Optionally, enable debugging information for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()
