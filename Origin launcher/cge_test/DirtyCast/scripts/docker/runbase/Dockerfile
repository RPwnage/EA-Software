###################################################################################################
# Author:  mclouatre
# Purpose: Creates an image that can later be used as a basis to run dirtycast processes.
#          Consists of a CentOS 7.4 base + some user/group creation
# Usage:   cd <...>\DirtyCast\scripts\docker\runbase
#          docker build --rm=true -t dirtycast/runbase:active .
# Output:  A new container image called "runbase" under the "dirtycast" repository in the list
#          of images known by the docker host. (use "docker images" to see it)
###################################################################################################

FROM library/ubuntu:bionic-20190912.1
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>

# prereq to add new repos
RUN apt-get update && apt-get install -y gnupg curl

RUN curl -fsSL https://packages.cloud.google.com/apt/doc/apt-key.gpg | apt-key add -
RUN echo "deb http://apt.kubernetes.io/ kubernetes-xenial main" | tee -a /etc/apt/sources.list.d/kubernetes.list

###################################################################################################
# Install boto3 (required for containers to be able to save log/core files to AWS S3 buckets) and dependencies
#   * python-pip        --> pip is a package management system used to install and manage software packages written in Python
#   * boto3             --> AWS SDK for Python
#   * libbz2-1.0        --> shared libraries for libbz2
# Install kubectl which can be used to query information and delete pods.
###################################################################################################

RUN apt-get update && apt-get install -y python-pip libbz2-1.0 kubectl
RUN pip install boto3

###################################################################################################
# Create the gs-tools user and gs-tools group
###################################################################################################

RUN groupadd -r gs-tools -g 627 && \
    useradd -u 627 -m -r -g gs-tools -d /home/gs-tools -s /sbin/nologin gs-tools && \
    chmod 755 /home/gs-tools


###################################################################################################
# Add the bash script used to install debug tools
###################################################################################################

ADD install_tools.sh /home/gs-tools/install_tools.sh
RUN chmod 744 /home/gs-tools/install_tools.sh


###################################################################################################
# Add the python script used to save core files and log files to AWS S3 upon container exiting
###################################################################################################

ADD backupToS3.py /home/gs-tools/backupToS3.py
RUN chmod 744 /home/gs-tools/backupToS3.py
