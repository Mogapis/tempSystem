cmake_minimum_required(VERSION 3.16)
project(sensor_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

find_library(WIRINGPI_LIB wiringPi REQUIRED)
find_library(CURL_LIB curl REQUIRED)
find_package(Threads REQUIRED)

add_executable(sensor_service
  src/main.cpp
  src/Dht22Reader.cpp
  src/Config.cpp
  src/HttpClient.cpp
)

target_include_directories(sensor_service PRIVATE src)
target_link_libraries(sensor_service PRIVATE ${WIRINGPI_LIB} ${CURL_LIB} Threads::Threads)