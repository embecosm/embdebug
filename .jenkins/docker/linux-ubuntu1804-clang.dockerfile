FROM ubuntu:18.04

LABEL maintainer simon.cook@embecosm.com

# Install required packages
RUN apt-get -y update && \
  DEBIAN_FRONTEND=noninteractive \
  apt-get install -y build-essential cmake

# Install clang toolchain, update alternatives
RUN DEBIAN_FRONTEND=noninteractive \
  apt-get install -y clang
RUN update-alternatives --set c++ /usr/bin/clang++ && \
  update-alternatives --set cc /usr/bin/clang
