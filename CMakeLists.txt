cmake_minimum_required(VERSION 3.10)

# Set the project name
project(AI_Resume)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get the repository for the json handling
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Find the curl library
find_package(CURL REQUIRED CONFIG)

# Add the executable
add_executable(AI_Resume main.cpp)  # Your source file

# Link the nlohmann_json and cURL libraries
target_link_libraries(AI_Resume PRIVATE nlohmann_json::nlohmann_json CURL::libcurl)
