FROM ubuntu:18.04

LABEL maintainer simon.cook@embecosm.com

# Install required packages
RUN apt-get -y update && \
  DEBIAN_FRONTEND=noninteractive \
  apt-get install -y build-essential git cmake

# Install clang toolchain, update alternatives
RUN DEBIAN_FRONTEND=noninteractive \
  apt-get install -y clang clang-format llvm
RUN update-alternatives --set c++ /usr/bin/clang++ && \
  update-alternatives --set cc /usr/bin/clang
