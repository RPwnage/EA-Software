###################################################################################################
# Author:  eesponda
# Purpose: gamestress container
#
# Usage when packaging internally synced source code using dockerindocker:
#   do not build by yourself - meant to be invoked from docker\packaging\dockerindocker.sh
#
# Usage when packaging build artificats obtained from pre-synced source code:
#    * Create the destination directory in the docker context:  mkdir -p ./gos-stream/ml/DirtyCast/stress/run
#    * Copy the stress client run directory: cp -r ../../../../../../stress/run ./gos-stream/ml/DirtyCast/stress/run
#    * docker build --build-arg DIRTYCAST_BINARY_ARG=<gsrvstressd|gsrvstressf> -t dirtycast/gsrvstress<d|f>_otp:latest .
#    * Remove the duplicated directory: rm -rf ./gos-stream
#
# Output:  A new container image called "gsrvstressd_otp" or "gsrvstressf_otp" under the "dirtycast" repository
#          in the list of images known by the docker host. (use "docker images" to see it)
#
#          Usage with Docker ToolBox VirtualBox VM:
#               docker run --name <container name> --env-file <name of file with env vars required by stress_launch.sh> -it dirtycast/gsrvstress<d|f>_otp:latest ./stress_launch.sh
###################################################################################################

ARG RUNBASE_IMAGE=dirtycast/runbase:active
FROM $RUNBASE_IMAGE
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>


###################################################################################################
# Define mandatory build arguments (shall be passed with --build-arg <name>=<value>)
###################################################################################################

ARG DIRTYCAST_BINARY_ARG


###################################################################################################
# Add the executable run-time directory to the container (has binary and config files) 
###################################################################################################

ADD ./DirtyCast/stress/run/stress.cfg /home/gs-tools/gamestress/stress.cfg
ADD ./DirtyCast/stress/run/$DIRTYCAST_BINARY_ARG /home/gs-tools/gamestress/$DIRTYCAST_BINARY_ARG
ADD ./DirtyCast/stress/run/libc++*.so.1 /usr/lib/x86_64-linux-gnu/


###################################################################################################
# Add env var identifying binary to be run
###################################################################################################

ENV SELECTED_GSRVSTRESS_BINARY=$DIRTYCAST_BINARY_ARG


###################################################################################################
# Add the stress launching script to be invoked when running the container. 
#################################################################################################

ADD stress_launch.sh /home/gs-tools/gamestress/stress_launch.sh
RUN chmod ugo+x /home/gs-tools/gamestress/stress_launch.sh
RUN chown -R gs-tools:gs-tools /home/gs-tools


###################################################################################################
# set the user
###################################################################################################

USER gs-tools


###################################################################################################
# Set container's working directory
#
# (no need to set "entrypoint" as the EADP cloud is explicitly specifying the cmd when running the container)
###################################################################################################

WORKDIR /home/gs-tools/gamestress/
