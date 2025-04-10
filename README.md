# NetCDF Reader

This repository contains a simple application for reading a NetCDF file via a REST API.

## Docker Build

Use the following command to build the application with Docker:

```
sudo docker build -t netcdf-reader .
```

## Docker Run

After building, run the container with the following command:

```
sudo docker run -p 18080:18080 netcdf-reader
```

The container will automatically open the application using the NetCDF file provided in the data directory.

## Usage

With the container running, open a browser page to <http://localhost:18080>

### Get Info

<http://localhost:18080/get-info> displays metadata for the NetCDF file.

### Get Data

To query the concentration data at a specific time and z-coordinate, go to <http://localhost:18080/get-data?t=t_index&z=z_index> (`t_index` and `z_index` should be replaced with the desired integer indices, e.g. <http://localhost:18080/get-data?t=0&z=0>).

As shown in /get-info, the number of time indices is 8 and the number of z indices is 1. This means that `t_index` can take values between 0 and 7 (inclusive) and `z_index` can only take a value of 0. Values outside these ranges will return an error.

### Get Image

To display the concentration data for a specific time and z-coordinate as a PNG image, go to <http://localhost:18080/get-image?t=t_index&z=z_index>. As with /get-data, `t_index` and `z_index` should be replaced with valid index values.

Due to the small x and y dimensions (36x27), the image will appear small.
