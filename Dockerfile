FROM jaschweder/php:server

MAINTAINER jonathan.schweder@bludata.com.br

RUN apt-get update -y \
    && apt-get upgrade -y \
    && apt-get install -y \
    git \
    build-essential \
    cmake \
    pkg-config \
    yasm \
    libx264-dev \
    libusb-dev \
    libmp3lame-dev \
    libasound2-dev \
    alsa-utils \
    alsa \
    alsa-oss \
    curl \
    intltool \
    libv4l-dev \
    libudev-dev \
    libusb-dev \
    libusb-1.0-0-dev \
    libsdl2-dev \
    libgsl-dev \
    portaudio19-dev

################################################################################
# VIDEO RECORDER
################################################################################

#install FFMPEG
RUN mkdir -p /usr/src \
    && cd /usr/src \
    && git clone -b n3.2.7 --depth 1 --single-branch https://github.com/ffmpeg/ffmpeg \
    && cd ffmpeg \
    && ./configure \
    --enable-libmp3lame \
    --enable-gpl \
    --enable-libx264 \
    --enable-shared \
    --disable-static \
    && make all -j$(nproc) \
    && make install \
    && cd /usr/local/include/ \
    && ln -s libavcodec/avcodec.h avcodec.h \
    && ln -s libavformat/avformat.h avformat.h \
    && ln -s libavio/avio.h avio.h \
    && ln -s libavutil/avutil.h avutil.h \
    && ln -s libswscale/swscale.h swscale.h

#install guvcview
COPY . /usr/src/guvcview

RUN cd /usr/src/guvcview \
    && ./configure \
    && make -j$(nproc) \
    && make install \
    && ldconfig

ENTRYPOINT ["guvcview"]
