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
    cd ..

# Set working directory

WORKDIR /

ENV PICO_SDK_PATH=/pico/pico-sdk

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
