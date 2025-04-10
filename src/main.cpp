#include <crow/app.h>
#include <crow/http_response.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <json.hpp>
#include <limits>
#include <netcdf>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

  CROW_ROUTE(app, "/get-data")
      .methods("GET"_method)([&nc_file](crow::request const& req) {
        auto result = json();

        try {
          char const* t_param = req.url_params.get("t");
          char const* z_param = req.url_params.get("z");

          if (not t_param or not z_param) {
            result["error"] = "Missing query parameters: t and z";
            return crow::response(400, result.dump());  // bad_request
          }

          auto t_index = static_cast<std::size_t>(std::stoi(t_param));
          auto z_index = static_cast<std::size_t>(std::stoi(z_param));

          auto x = nc_file.getVar("x");
          if (x.isNull()) {
            result["error"] = "Variable 'x' not found.";
            return crow::response(500, result.dump());  // internal_server_error
          }
          auto y = nc_file.getVar("y");
          if (y.isNull()) {
            result["error"] = "Variable 'y' not found.";
            return crow::response(500, result.dump());  // internal_server_error
          }
          auto concentration = nc_file.getVar("concentration");
          if (concentration.isNull()) {
            result["error"] = "Variable 'concentration' not found.";
            return crow::response(500, result.dump());  // internal_server_error
          }

          auto row_size = nc_file.getDim("x").getSize();
          auto x_values = std::vector<double>(row_size);
          x.getVar(x_values.data());

          auto col_size = nc_file.getDim("y").getSize();
          auto y_values = std::vector<double>(col_size);
          y.getVar(y_values.data());

          auto start = std::vector<std::size_t>{t_index, z_index, 0, 0};
          auto count = std::vector<std::size_t>{1, 1, col_size, row_size};
          auto concentration_data = std::vector<double>(row_size * col_size);
          concentration.getVar(start, count, concentration_data.data());

          // make nested array
          auto grid = json::array();
          for (std::size_t row_idx = 0; row_idx < col_size; ++row_idx) {
            auto row = json::array();
            for (std::size_t col_idx = 0; col_idx < row_size; ++col_idx) {
              // Array of structs is chosen for display purposes. Struct of
              // arrays may be preferred if the purpose is to read the data into
              // data structures.
              row.push_back(
                  {{"x", x_values[col_idx]},
                   {"y", y_values[row_idx]},
                   {"concentration",
                    concentration_data[row_idx * row_size + col_idx]}});
            }
            grid.push_back(row);
          }

          result["concentration_data"] = grid;
        } catch (std::exception const& e) {
          result["error"] = e.what();
          return crow::response(500, result.dump(2));  // internal_server_error
        }

        return crow::response(result.dump(2));
      });

  CROW_ROUTE(app, "/get-image")
      .methods("GET"_method)([&nc_file](crow::request const& request) {
        auto result = json();
        try {
          char const* t_param = request.url_params.get("t");
          char const* z_param = request.url_params.get("z");

          if (not t_param or not z_param) {
            result["error"] = "Missing query parameters: t and z";
            return crow::response(400, result.dump());  // bad_request
          }

          auto t_index = static_cast<std::size_t>(std::stoi(t_param));
          auto z_index = static_cast<std::size_t>(std::stoi(z_param));

          auto x = nc_file.getVar("x");
          if (x.isNull()) {
            result["error"] = "Variable 'x' not found.";
            return crow::response(500, result.dump());  // internal_server_error
          }
          auto y = nc_file.getVar("y");
          if (y.isNull()) {
            result["error"] = "Variable 'y' not found.";
            return crow::response(500, result.dump());  // internal_server_error
          }
          auto concentration = nc_file.getVar("concentration");
          if (concentration.isNull()) {
            result["error"] = "Variable 'concentration' not found.";
            return crow::response(500, result.dump());  // internal_server_error
          }

          auto row_size = nc_file.getDim("x").getSize();
          auto col_size = nc_file.getDim("y").getSize();
          auto start = std::vector<std::size_t>{t_index, z_index, 0, 0};
          auto count = std::vector<std::size_t>{1, 1, col_size, row_size};
          auto concentration_data = std::vector<double>(row_size * col_size);
          concentration.getVar(start, count, concentration_data.data());

          // color range
          auto min_value = *std::min_element(concentration_data.begin(),
                                             concentration_data.end());
          auto max_value = *std::max_element(concentration_data.begin(),
                                             concentration_data.end());
          auto value_range = max_value - min_value;

          // compute pixel values
          auto image = std::vector<uint8_t>(row_size * col_size);
          for (std::size_t i = 0; i < concentration_data.size(); ++i) {
            image[i] = static_cast<uint8_t>(
                std::numeric_limits<uint8_t>::max() *
                ((concentration_data[i] - min_value) / value_range));
          }

          // encode PNG
          auto png = std::vector<uint8_t>{};
          stbi_write_png_to_func(
              [](void* context, void* data, int size) {
                auto* buffer = static_cast<std::vector<uint8_t>*>(context);
                buffer->insert(buffer->end(), static_cast<uint8_t*>(data),
                               static_cast<uint8_t*>(data) + size);
              },
              &png, row_size, col_size, 1, image.data(), row_size);

          auto response = crow::response{};
          response.set_header("Content-Type", "image/png");
          response.body = std::string(png.begin(), png.end());
          response.code = 200;  // successful
          return response;
        } catch (std::exception const& e) {
          result["error"] = e.what();
          return crow::response(500, result.dump(2));  // internal_server_error
        }
      });

  app.port(18080).multithreaded().run();
}
