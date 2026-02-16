FROM debian:trixie

RUN apt update

# Dependencies of esp-idf
RUN apt -y install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone esp-idf
RUN git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git

# Install tools used by esp-idf for esp32s3
WORKDIR /esp-idf
RUN ./install.sh esp32s3 > install-sh.log 2>&1
WORKDIR /

ENV IDF_PATH=/esp-idf
ENTRYPOINT ["/esp-idf/tools/docker/entrypoint.sh"]

# Install pico sdk required dependencies
RUN apt update && \
    apt install -y git python3 python3-pip cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential xxd && \
    mkdir -p pico && \
    cd pico && \
    git clone https://github.com/raspberrypi/pico-sdk.git --branch 2.1.1 && \
    cd pico-sdk/ && \
    git submodule update --init && \
    cd ../.. && \
    \
    git clone https://github.com/emscripten-core/emsdk.git && \
    cd emsdk && \
    git pull && \
    ./emsdk install latest && \
    ./emsdk activate latest && \
    . ./emsdk_env.sh && \
    cd ..

WORKDIR /

ENV PICO_SDK_PATH=/pico/pico-sdk

# Install picotool
RUN apt install -y libusb-1.0-0-dev
RUN git clone --depth 1 --branch 2.1.1 https://github.com/raspberrypi/picotool.git
RUN mkdir -p picotool/build
WORKDIR /picotool
RUN cmake . -B build
WORKDIR /picotool/build
RUN make && cmake --install .

WORKDIR /

ENV EMSDK=/emsdk EM_CONFIG=/emsdk/.emscripten EMSDK_NODE=/emsdk/node/14.18.2_64bit/bin/node PATH=/emsdk:/emsdk/upstream/emscripten:/emsdk/upstream/bin:/emsdk/node/14.18.2_64bit/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

RUN apt update && \
    apt install -y socat

# Install pre-commit from pip
RUN python3 -m pip install --break-system-packages pre-commit
RUN pre-commit --version

# Copy pre-commit hooks and create a git directory,
# to allow missing environments of hooks to be installed
COPY ./.pre-commit-config.yaml /
RUN git init
RUN pre-commit install-hooks

# Add /project as a safe git repository
RUN git config --global --add safe.directory /project

# Patch FreeRTOSConfig.h
COPY ./patch_esp_trace_include.sh /
RUN ./patch_esp_trace_include.sh

# Define default command
CMD ["bash"]
