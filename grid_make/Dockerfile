FROM ubuntu:20.04

WORKDIR /usr/grid_make

RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

# general setup
RUN apt-get update && \
    apt-get -y install vim wget sudo git make cmake  && \
    apt-get clean

# User account
ENV USER=ubuntu

# Setup default user
RUN useradd --create-home -s /bin/bash -m $USER && echo "$USER:ubuntu" | chpasswd && adduser $USER sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

WORKDIR /home/$USER
USER $USER

# Install Cortex-R and Cortex-M bare metal GNU tools
RUN wget -qO-  https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2 | tar xj
ENV PATH "/home/$USER/gcc-arm-none-eabi-9-2020-q2-update/bin:$PATH"

# Install gdb dependencies
RUN sudo apt-get -y install libncurses5
