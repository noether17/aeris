#include <crow/app.h>
#include <crow/http_response.h>

#include <cstdlib>
#include <iostream>
#include <netcdf>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
  // Read input filename from command line
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << "NetCDF-filename\n";
    return EXIT_FAILURE;
  }
  auto nc_filename = std::string{argv[1]};

  // open the file
  auto nc_file = [&nc_filename] {
    try {
      return netCDF::NcFile{nc_filename, netCDF::NcFile::read};
    } catch (netCDF::exceptions::NcNotNCF const& e) {
      std::cerr << e.what() << '\n';
      exit(EXIT_FAILURE);
    }
  }();

  crow::SimpleApp app;

  CROW_ROUTE(app, "/")([] { return "Hello world"; });

  CROW_ROUTE(app, "/get-info")([&nc_file] {
    // dimensions
    auto dims = json::object();
    for (auto const& [dim_name, dim] : nc_file.getDims()) {
      dims[dim_name] = dim.getSize();
    }

    // variables
    auto vars = json::object();
    for (auto const& [var_name, var] : nc_file.getVars()) {
      auto var_info = json{};
      var_info["type"] = var.getType().getName();

      auto dim_names = std::vector<std::string>{};
      for (auto const& dim : var.getDims()) {
        dim_names.push_back(dim.getName());
      }
      var_info["dimensions"] = dim_names;

      auto attributes = json::object();
      for (auto const& [attribute_name, attribute] : var.getAtts()) {
        auto value = std::string{};
        attribute.getValues(value);
        attributes[attribute_name] = value;
      }
      var_info["attributes"] = attributes;

      vars[var_name] = var_info;
    }

    // result
    auto result = json{};
    result["dimensions"] = dims;
    result["variables"] = vars;
    return crow::response(result.dump(2));
  });

  app.port(18080).multithreaded().run();
}
