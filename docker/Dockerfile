FROM nvidia/cuda:12.3.2-runtime-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

### mitsuba dependencies

RUN apt update \
    && apt install -y \
        apt-transport-https \
    && apt clean \
    && apt autoclean \
    && apt autoremove 

RUN apt update \
    && apt upgrade -y \
    && apt install -y \
        unzip \
        wget \
        ca-certificates \
        software-properties-common \
        lsb-release \
        kmod \
    && apt clean \
    && apt autoclean \
    && apt autoremove

WORKDIR /StatMC