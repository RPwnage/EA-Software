###################################################################################################
# Author:  amakoukji
# Purpose: qosstress container
#
# Usage when packaging internally synced source code using dockerindocker:
#   do not build by yourself - meant to be invoked from docker\packaging\dockerindocker.sh
#
# Usage when packaging build artificats obtained from pre-synced source code:
#    * Create the destination directory in the docker context:  mkdir -p ./gos-stream/ml/DirtyCast/qosstress/run
#    * Copy the qosstress run directory: cp -r ../../../../../../qosstress/run ./gos-stream/ml/DirtyCast/qosstress/run
#    * docker build --build-arg DIRTYCAST_BINARY_ARG=<qosstressd|qosstressf> -t dirtycast/qosstress<d|f>:latest .
#    * Remove the duplicated directory: rm -rf ./gos-stream
#
# Output:  A new container image called "qosstressd_qos" or "qosstressf_qos" under the "dirtycast" repository
#          in the list of images known by the docker host. (use "docker images" to see it) 
#
#          Usage with Docker ToolBox VirtualBox VM:
#               docker run --name <container name> --env-file <name of file with env vars required by stress_launch.sh> --net=host -it dirtycast/qosstress<d|f>_qos:latest ./stress_launch.sh
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

ADD ./DirtyCast/qosstress/run/validate.cfg /home/gs-tools/qosstress/validate.cfg
ADD ./DirtyCast/qosstress/run/qosstress_logger.cfg /home/gs-tools/qosstress/qosstress_logger.cfg
ADD ./DirtyCast/qosstress/run/$DIRTYCAST_BINARY_ARG /home/gs-tools/qosstress/$DIRTYCAST_BINARY_ARG
ADD ./DirtyCast/qosstress/run/libc++*.so.1 /usr/lib/x86_64-linux-gnu/


###################################################################################################
# Add env var identifying binary to be run
###################################################################################################

ENV SELECTED_QOSSTRESS_BINARY=$DIRTYCAST_BINARY_ARG


###################################################################################################
# Add the stress launching script to be invoked when running the container. 
#################################################################################################

ADD stress_launch.sh /home/gs-tools/qosstress/stress_launch.sh
RUN chmod ugo+x /home/gs-tools/qosstress/stress_launch.sh
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

WORKDIR /home/gs-tools/qosstress/
