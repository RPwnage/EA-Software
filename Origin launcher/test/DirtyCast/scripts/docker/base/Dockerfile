###################################################################################################
# Author:  mclouatre
# Purpose: Creates a base image that can later be used as a basis to build DirtyCast source code
#          (build step), and additionally package the build output into a lightweight prod ready
#          container (packaging step).
#          Consists of a CentOS 7.4 base + miscellaneous tools
# Usage:   cd <...>\DirtyCast\scripts\docker\base
#          docker build -t dirtycast/base:active .
# Output:  A new container image called "base" under the "dirtycast" repository in the list
#          of images known by the docker host. (use "docker images" to see it)
###################################################################################################

FROM library/ubuntu:bionic-20200921
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>

# -------------------------------------------------------------------------------------------------
# PART 1 - Installing software required to build DirtyCast (build step)
# -------------------------------------------------------------------------------------------------

###################################################################################################
# Local configuration of required apt repos:
#   * create config file for mono-official-stable (for: mono-devel)
#   * create config file for perforce (for: helix)
###################################################################################################

# prereq to add new repos
RUN apt-get update && apt-get install -y gnupg curl

RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
RUN echo "deb https://download.mono-project.com/repo/ubuntu bionic/snapshots/6.8 main" | tee /etc/apt/sources.list.d/mono-official-stable.list

RUN curl -fsSL https://package.perforce.com/perforce.pubkey | apt-key add -
RUN echo "deb http://package.perforce.com/apt/ubuntu bionic release" | tee /etc/apt/sources.list.d/perforce.list

###################################################################################################
# Install required tools:
#   * build-essential           --> build tools for development
#   * openjdk-8-jdk-headless    --> required by EATDF
#   * mono-devel                --> framework requires more recent versions of mono
#   * bzip2                     --> required by dirtycast/logger which includes bzlib.h
#   * zlib                      --> required by dirtycast/logger which includes zlib.h
#   * lsb-core                  --> required by build tool chain to figure out what version of OS we are using
#   * helix-cli (p4 client)     --> required by dirtycast/scripts/mkver.xml, and needed to optionally sync source code "in" the container
###################################################################################################

RUN apt-get update && \
    apt-get install -y build-essential \
                       openjdk-8-jdk-headless \
                       mono-devel \
                       libz-dev \
                       libbz2-dev \
                       lsb-core \
                       helix-cli


# -------------------------------------------------------------------------------------------------
# PART 2 - Installing software required for creating the DirtyCast container  (packaging step)
# -------------------------------------------------------------------------------------------------

###################################################################################################
# Install docker.io   (needed because we want to exercise docker client internally)
###################################################################################################

RUN apt-get install -y docker.io
