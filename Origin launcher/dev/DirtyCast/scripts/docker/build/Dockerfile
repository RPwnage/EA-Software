###################################################################################################
# Author:  mclouatre
# Purpose: Creates an image that can be instantiated to build DirtyCast with the syncandbuild.sh 
#          script which syncs the DirtyCast source code internally and builds it, or with the
#          build.sh script which would builds pre-synced code.
#
# Usage when source code is internally synced:
#          cd <...>\DirtyCast\scripts\docker\build
#          docker build -t dirtycast/build:active .
#          docker run --name <container name> -e P4USER=<p4 username ex: EAMONTREAL\\mclouatre> -it dirtycast/build:active ./syncandbuild.sh
#           !!!INTERACTIVE!!! container_prompt> type in your p4 password when prompted for it
#          docker commit -a <you@ea.com> <container name> dirtycast/buildoutput:active  --> to create
#              a base image that contains dirtycast binaries and that can then be used as a basis
#              to create minimal lightweight production containers.
#
# Usage when source code is pre-synced and passed as a mounted directory:
#          cd <...>\DirtyCast\scripts\docker\build
#          docker build -t dirtycast/build:active .
#          docker run --name <container name> -v <host source code dir ex: ~/p4/f-dirtycast-containerization>:/tmp/EAGS/gos-stream/ml -it dirtycast/build:active ./build.sh
#          docker commit -a <you@ea.com> <container name> dirtycast/buildoutput:active  --> to create
#              a base image that contains dirtycast binaries and that can then be used as a basis
#              to create minimal lightweight production containers.
#
# Output:  After "docker build":
#              A new container image called "build" under the "dirtycast" repository in the list
#              of images known by the docker host. (use "docker images" to see it)
#          After "docker commit": 
#              A new container image called "buildoutput" under the "dirtycast" repository in the list
#              of images known by the docker host. (use "docker images" to see it)
###################################################################################################

FROM dirtycast/base:active
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>


###################################################################################################
# Define optional build arguments (can be overriden with --build-arg <name>=<value>)
###################################################################################################

ARG ARG_P4WORKSPACE_DIRTYCAST=dirtycast_docker_build_sync_ml
ARG USER_ID=1000
ARG GROUP_ID=1000

###################################################################################################
# Create working directory for build process
###################################################################################################

# create /tmp/EAGS
RUN mkdir /tmp/EAGS/ && \
    mkdir /tmp/EAGS/gos-stream

###################################################################################################
# Create the gs-tools user and gs-tools group (under same user who is building)
###################################################################################################

RUN groupadd gs-tools -g ${GROUP_ID} && \
    useradd -l -u ${USER_ID} -g gs-tools gs-tools && \
    install -d -m 0755 -o gs-tools -g gs-tools /home/gs-tools && \
    chown -c -R ${USER_ID}:${GROUP_ID} /tmp/EAGS /home/gs-tools
USER gs-tools 

###################################################################################################
# Add bash scripts automating sync and build phase.
###################################################################################################

ADD sync.sh /tmp/EAGS/sync.sh
ADD build.sh /tmp/EAGS/build.sh
ADD build-stress.sh /tmp/EAGS/build-stress.sh
ADD syncandbuild.sh /tmp/EAGS/syncandbuild.sh


###################################################################################################
# Add env vars
###################################################################################################

ENV P4PORT=eadp-perforce.rws.ad.ea.com:1777 P4CHARSET=utf8
ENV P4WORKSPACE_DIRTYCAST=$ARG_P4WORKSPACE_DIRTYCAST


###################################################################################################
# Set working directory
###################################################################################################

WORKDIR /tmp/EAGS
