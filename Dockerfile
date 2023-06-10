FROM nvcr.io/nvidia/l4t-base:r35.1.0 as base
LABEL maintainer="David Jablonski <dayjaby@gmail.com>"

ENV DEBIAN_FRONTEND noninteractive
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

RUN apt-get update && apt-get -y --quiet --no-install-recommends install \
                build-essential \
                bzip2 \
                ca-certificates \
                ccache \
                cmake \
                curl \
                file \
                g++ \
                gcc \
                gnupg \
                libopencv-dev \
                libopencv-contrib-dev \
                lsb-release \
                make \
                pkg-config \
                unzip \
                wget \
                zip \
        && apt-get -y autoremove \
        && apt-get clean autoclean \
        && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

COPY main.cpp /root/main.cpp
WORKDIR /root/build
RUN cmake .. \
    make -j$(nproc) \
    ./rtspserver

EXPOSE 8554
