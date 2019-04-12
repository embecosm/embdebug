FROM centos/devtoolset-6-toolchain-centos7

LABEL maintainer simon.cook@embecosm.com

# Software collection images set the USER, run the following commands as root,
# so packages can be installed
USER 0

# Enable EPEL and install required packages
RUN yum -y install epel-release
RUN yum -y upgrade && yum install -y cmake3 make git
RUN ln -s /usr/bin/cmake3 /usr/bin/cmake
