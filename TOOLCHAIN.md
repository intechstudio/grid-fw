Install podman

sudo apt-get -y install podman

Edit container registries

sudo nano /etc/containers/registries.conf

Add docker hub to the end of the file

[registries.search]
registries = ['docker.io']

Login to dockerhub using

podman login docker.io

Pull the arm-none-eabi-gcc image

podman pull srzzumix/arm-none-eabi:focal-9-2019q4

Pull the esp-idf image

podman pull espressif/idf:latest

Pull the RP2040 SDK image

podman pull atoktoto/pico-builder:latest