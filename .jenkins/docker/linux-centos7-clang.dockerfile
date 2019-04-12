FROM centos:7

LABEL maintainer simon.cook@embecosm.com

# Enable EPEL and install required packages
RUN yum -y install epel-release
RUN yum -y upgrade && yum install -y cmake3 make gcc gcc-c++ git
RUN ln -s /usr/bin/cmake3 /usr/bin/cmake

# Install clang and make the default compiler
RUN yum -y install clang
RUN ln -sf clang /usr/bin/cc && ln -sf clang++ /usr/bin/c++
