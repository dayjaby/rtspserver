FROM nvcr.io/nvidia/l4t-base:r35.1.0 as base
LABEL maintainer="David Jablonski <dayjaby@gmail.com>"

ENV DEBIAN_FRONTEND noninteractive
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

COPY opencv_arm64.deb /tmp
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
                libgstreamer1.0-dev \
                libgstrtspserver-1.0-dev \
                lsb-release \
                make \
                pkg-config \
                unzip \
                wget \
                zip \
        && dpkg -i /tmp/opencv_arm64.deb \
        && apt-get -y autoremove \
        && apt-get clean autoclean \
        && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY main.cpp /root/main.cpp
COPY CMakeLists.txt /root/CMakeLists.txt
WORKDIR /root/build
RUN cmake .. && \
    make -j$(nproc)

EXPOSE 8554
CMD ["/root/build/rtspserver"]

