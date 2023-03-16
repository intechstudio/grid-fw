ARG UBUNTU_VERSION=latest
FROM ubuntu:$UBUNTU_VERSION
ARG UBUNTU_VERSION

ARG DIR=9-2019q4
ARG FILE=gcc-arm-none-eabi-9-2019-q4-major
ARG OUTFILE=gcc-arm-none-eabi-9-2019-q4-major
ARG ARCHI=x86_64-linux

LABEL maintainer "srz_zumix <https://github.com/srz-zumix>"

ENV DEBIAN_FRONTEND=noninteractive
# https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
RUN dpkg --add-architecture i386 && \
  apt-get update && apt-get -y --no-install-recommends install \
    ca-certificates \
    build-essential clang libc6:i386 \
    bzip2 \
    git \
    cmake make \
    python3 \
    vim-common astyle \
    wget \
    && \
  apt-get clean && \
  rm -rf /var/lib/apt/lists/* && \
  mkdir -p /usr/local/ && \
  wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/$DIR/$FILE-$ARCHI.tar.bz2 && \
  tar -xf $FILE-$ARCHI.tar.bz2 -C /usr/local/ && rm *.tar.bz2 && \
  apt-get clean

ENV PATH $PATH:/usr/local/$OUTFILE/bin
ENV CC=arm-none-eabi-gcc \
    CXX=arm-none-eabi-g++ \
    CMAKE_C_COMPILER=arm-none-eabi-gcc \
    CMAKE_CXX_COMPILER=arm-none-eabi-g++ \
    STRIP=arm-none-eabi-strip \
    RANLIB=arm-none-eabi-ranlib \
    AS=arm-none-eabi-as \
    AR=arm-none-eabi-ar \
    LD=arm-none-eabi-ld \
    FC=arm-none-eabi-gfortran
ENV LD_LIBRARY_PATH /usr/local/$OUTFILE/lib:$LD_LIBRARY_PATH

WORKDIR /grid-fw/grid_make/gcc
