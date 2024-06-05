cmake_minimum_required(VERSION 3.10)
project(UniversalAPIGateway)

set(CMAKE_CXX_STANDARD 17)

# Add vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Find Crow and cURL
find_package(Crow CONFIG REQUIRED)
find_package(CURL REQUIRED)

# Add executable
add_executable(UniversalAPIGateway main.cpp)

# Link libraries
target_link_libraries(UniversalAPIGateway PRIVATE Crow::Crow ${CURL_LIBRARIES})
