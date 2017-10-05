FROM ubuntu

MAINTAINER jonathan.schweder@bludata.com.br

RUN apt-get update -y \
    && apt-get upgrade -y \
    && apt-get install \
    build-essential \
    cmake \
    pkg-config \
    intltool \
    autotools-dev \
    libsdl2-dev \
    libsfml-dev \
    portaudio19-dev \
    libpng12-dev \
    libavcodec-dev \
    libavutil-dev \
    libv4l-dev \
    libudev-dev \
    libusb-1.0-0-dev \
    libpulse-dev \
    libgsl0-dev \
    libtool \
    --no-install-recommends \
    --no-install-suggests \
    -y

COPY . /usr/src/guvcview

RUN cd /usr/src/guvcview \
    && ./bootstrap.sh \
    && ./configure \
    && make -j$(nproc) \
    && make install \
    && ldconfig

ENTRYPOINT ["guvcview"]
