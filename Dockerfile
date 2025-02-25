# usage:
# docker container rm yurots_builder
# docker build -t yurots_builder -f Dockerfile .
# docker run --name yurots_builder -it -v "$(pwd)/ots:/ots" yurots_builder
# cd /ots
# bash build.sh
FROM --platform=linux/386 ubuntu:16.04
LABEL name="yurots_builder"
LABEL description="Docker image for building yurots on Ubuntu 16.04 (x86-64)"
LABEL version="1.0"
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    libboost-all-dev \
    liblua5.1-dev \
    libmysqlclient-dev \
    libssl-dev \
    libxml2-dev \
    libsqlite3-dev \
    pkg-config \
    git \
    wget \
    curl \
    unzip \
    libxml2-dev
# Set working directory
ENV CPLUS_INCLUDE_PATH=/usr/include/lua5.1/:/usr/include/libxml2/:$CPLUS_INCLUDE_PATH
WORKDIR /ots/source/devcpp
CMD ["/bin/bash"]
