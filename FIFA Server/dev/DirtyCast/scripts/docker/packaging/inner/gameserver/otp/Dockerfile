###################################################################################################
# Author:  mclouatre
# Purpose: gameserver container
#
# Usage when packaging internally synced source code using dockerindocker:
#   do not build by yourself - meant to be invoked from docker\packaging\dockerindocker.sh
#
# Usage when packaging build artificats obtained from pre-synced source code:
#    * Create the destination directory in the docker context:  mkdir -p ./gos-stream/ml/DirtyCast/GameServer/run
#    * Copy the gameserver run directory: cp -r ../../../../../../GameServer/run ./gos-stream/ml/DirtyCast/GameServer/run
#    * docker build --build-arg DIRTYCAST_BINARY_ARG=<gameserverd|gameserverf> -t dirtycast/gameserver<d|f>_cc:latest .
#    * Remove the duplicated directory: rm -rf ./gos-stream
#
# Output:  A new container image called "gameserverd_otp" or "gameserverf_otp" under the "dirtycast" repository
#          in the list of images known by the docker host. (use "docker images" to see it) 
#
#          Usage with Docker ToolBox VirtualBox VM:
#               docker run --name <container name> --env-file <name of file with env vars required by gameserver_launch.sh> --net=host -it dirtycast/gameserver<d|f>_otp:latest ./gameserver_launch.sh
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

ADD ./DirtyCast/GameServer/run/gameserver.cfg /home/gs-tools/gameserver/gameserver.cfg
ADD ./DirtyCast/GameServer/run/gameserver_logger.cfg /home/gs-tools/gameserver/gameserver_logger.cfg
ADD ./DirtyCast/GameServer/run/$DIRTYCAST_BINARY_ARG /home/gs-tools/gameserver/$DIRTYCAST_BINARY_ARG
ADD ./DirtyCast/GameServer/run/libc++*.so.1 /usr/lib/x86_64-linux-gnu/


###################################################################################################
# Add env var identifying binary to be run
###################################################################################################

ENV SELECTED_GAMESERVER_BINARY=$DIRTYCAST_BINARY_ARG


###################################################################################################
# Add the gameserver launching script to be invoked when running the container. 
#################################################################################################

ADD gameserver_launch.sh /home/gs-tools/gameserver/gameserver_launch.sh
RUN chmod ugo+x /home/gs-tools/gameserver/gameserver_launch.sh
RUN chown -R gs-tools:gs-tools /home/gs-tools

###################################################################################################
# Add the supervisord configuration used for launching supervisord
#################################################################################################

ADD supervisord.conf /etc/supervisor/supervisord.conf

###################################################################################################
# set the user
###################################################################################################

USER gs-tools


###################################################################################################
# Set container's working directory
#
# (no need to set "entrypoint" as the EADP cloud is explicitly specifying the cmd when running the container)
###################################################################################################

WORKDIR /home/gs-tools/gameserver/
