FROM ubuntu:16.04

LABEL maintainer simon.cook@embecosm.com

# Install required packages
RUN apt-get -y update && \
  DEBIAN_FRONTEND=noninteractive \
  apt-get install -y build-essential git cmake
