add_executable(main main.cpp)
target_include_directories(main PRIVATE include)
target_link_libraries(main PRIVATE
  netcdf
  netcdf_c++4
  nlohmann_json::nlohmann_json
)
