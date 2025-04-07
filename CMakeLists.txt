cmake_minimum_required(VERSION 4.0)
project(Aeris)

find_package(netCDF REQUIRED)
find_package(nlohmann_json REQUIRED)

add_subdirectory(src)
