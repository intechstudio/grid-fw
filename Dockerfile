# Use the base image
FROM docker.io/espressif/idf:v5.3.1

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

# Install necessary packages for getting the latest CMAKE including software-properties-common
# 3. Update the package list and install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libtool \
    autoconf \
    unzip \
    wget \
    && apt-get clean

# 4. Set variables for CMake version and build
ENV CMAKE_VERSION=3.28 \
    CMAKE_BUILD=1

# 5. Create a temporary directory and download CMake
RUN mkdir /temp && cd /temp \
    && wget https://cmake.org/files/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.$CMAKE_BUILD.tar.gz \
    && tar -xzvf cmake-$CMAKE_VERSION.$CMAKE_BUILD.tar.gz

# 6. Build and install CMake
RUN cd /temp/cmake-$CMAKE_VERSION.$CMAKE_BUILD \
    && ./bootstrap \
    && make -j$(nproc) \
    && make install

# 7. Verify CMake installation
RUN cmake --version

# 8. Clean up temporary files
RUN rm -rf /temp



# Set working directory

WORKDIR /

ENV PICO_SDK_PATH=/pico/pico-sdk

ENV EMSDK=/emsdk EM_CONFIG=/emsdk/.emscripten EMSDK_NODE=/emsdk/node/14.18.2_64bit/bin/node PATH=/emsdk:/emsdk/upstream/emscripten:/emsdk/upstream/bin:/emsdk/node/14.18.2_64bit/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
# Set up environment variables
ENV CODEQL_HOME=/opt/codeql
ENV PATH="${CODEQL_HOME}/codeql:${PATH}"

# Install CodeQL CLI tools
RUN mkdir -p ${CODEQL_HOME} && \
    curl -L https://github.com/github/codeql-cli-binaries/releases/latest/download/codeql-linux64.zip -o ${CODEQL_HOME}/codeql.zip && \
    unzip ${CODEQL_HOME}/codeql.zip -d ${CODEQL_HOME} && \
    rm ${CODEQL_HOME}/codeql.zip && \
    codeql --version

RUN cd ${CODEQL_HOME} && git clone --recursive https://github.com/github/codeql.git codeql-repo

RUN apt update && \
    apt install -y socat

# Define default command
CMD ["bash"]
