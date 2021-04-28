###################################################################################################
# Author:  mclouatre
# Purpose: voipserver (otp mode) container
#
# Usage when packaging internally synced source code using dockerindocker:
#   do not build by yourself - meant to be invoked from docker\packaging\dockerindocker.sh
#
# Usage when packaging build artificats obtained from pre-synced source code:
#    * Create the destination directory in the docker context:  mkdir -p ./gos-stream/ml/DirtyCast/run
#    * Copy the voipserver run directory: cp -r ../../../../../../run ./gos-stream/ml/DirtyCast/run
#    * docker build --build-arg DIRTYCAST_BINARY_ARG=<voipserverd|voipserverf> -t dirtycast/voipserver<d|f>_cc:latest .
#    * Remove the duplicated directory: rm -rf ./gos-stream
#
# Output:  A new container image called "voipserverd_otp" or "voipserverf_otp" under the "dirtycast" repository
#          in the list of images known by the docker host. (use "docker images" to see it) 
#
#          Usage with Docker ToolBox VirtualBox VM:
#               docker run --name <container name> --env-file <name of file with env vars required by voipserver_launch.sh> --net=host -it dirtycast/voipserver<d|f>_otp:latest ./voipserver_launch.sh
###################################################################################################

ARG RUNBASE_IMAGE=dirtycast/runbase:active
FROM $RUNBASE_IMAGE
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>


###################################################################################################
# Define mandatory build arguments (shall be passed with --build-arg <name>=<value>)
###################################################################################################

ARG DIRTYCAST_BINARY_ARG


###################################################################################################
# Add the voipserver run-time directory to the container (has binary and config files) 
###################################################################################################

ADD ./DirtyCast/run/voipserver.cfg /home/gs-tools/voipserver/voipserver.cfg
ADD ./DirtyCast/run/voipserver_logger.cfg /home/gs-tools/voipserver/voipserver_logger.cfg
ADD ./DirtyCast/run/$DIRTYCAST_BINARY_ARG /home/gs-tools/voipserver/$DIRTYCAST_BINARY_ARG
ADD ./DirtyCast/run/libc++*.so.1 /usr/lib/x86_64-linux-gnu/


###################################################################################################
# Add env var identifying binary to be run
###################################################################################################

ENV SELECTED_VOIPSERVER_BINARY=$DIRTYCAST_BINARY_ARG


###################################################################################################
# Add the voipserver launching script to be invoked when running the container. 
#################################################################################################

ADD voipserver_launch.sh /home/gs-tools/voipserver/voipserver_launch.sh
RUN chmod ugo+x /home/gs-tools/voipserver/voipserver_launch.sh
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

WORKDIR /home/gs-tools/voipserver/

