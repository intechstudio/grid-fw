# Use the base image
FROM docker.io/espressif/idf:v5.1.2

# Install pico sdk required dependencies
RUN apt update && \
    apt install -y git python3 cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential xxd && \
    mkdir -p pico && \
    cd pico && \
    git clone https://github.com/raspberrypi/pico-sdk.git --branch master && \
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

# Set working directory
WORKDIR /

ENV PICO_SDK_PATH=/pico/pico-sdk

ENV EMSDK=/emsdk EM_CONFIG=/emsdk/.emscripten EMSDK_NODE=/emsdk/node/14.18.2_64bit/bin/node PATH=/emsdk:/emsdk/upstream/emscripten:/emsdk/upstream/bin:/emsdk/node/14.18.2_64bit/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

# Define default command
CMD ["bash"]
