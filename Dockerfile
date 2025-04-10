FROM ubuntu:24.04

# install dependencies
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  git \
  libasio-dev \
  libnetcdf-dev \
  libnetcdf-c++4-dev \
  python3

# install Crow
RUN git clone https://github.com/CrowCpp/Crow.git \
  && cd Crow \
  && mkdir build \
  && cd build \
  && cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF \
  && make install

WORKDIR /app

COPY . .

# build app
RUN mkdir build \
  && cd build \
  && cmake .. \
  && cmake --build .

EXPOSE 18080

CMD ["./build/src/main", "data/concentration.timeseries.nc"]
