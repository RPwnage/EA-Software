###################################################################################################
# Author:  mclouatre
# Purpose: Creates an stunnel container image with minimal foot print and ready for deployment. 
#          Requires the same runbase image as the one used by DirtyCast processes (dirtycast/runbase:latest).
# Usage:   cd <...>\DirtyCast\scripts\docker\stunnel
#          docker build --rm=true -t dirtycast/stunnel:<tag> .
# Output:  A new container image called "stunnel" under the "dirtycast" repository in the list
#          of images known by the docker host. (use "docker images" to see it)
#          Usage with Docker ToolBox VirtualBox VM:
#             docker run --name <container name> --net=host -it \
#                        --env-file <name of file with env vars required by stunnel_launch.sh> \
#                        -v <example:/home/gos-ops/cert/gos2015>:/etc/stunnel-secret/server-cert \
#                        -v <example:~p4/ml/DirtyCast/run/certs>:/etc/stunnel-secret/ca-cert \
#                        dirtycast/stunnel:<tag>
# Notes:   "-v <host dir with server certs>:/etc/stunnel-secret/server-cert" is required for stunnel to have access 
#          to server cert (cert.pem) and server private key (key.pem) of the server box running the docker host.
#          For test purposes, create a self-signed cert (https://www.stunnel.org/howto.html) and copy it (with the private key) to your VirtualBox VM
#
#          "-v <host dir with ca cert>:/etc/stunnel-secret/ca-cert" is required for stunnel to have access
#          to ca cert (cacert.pem). 
#          (The cacert currently archived in p4 can be used: /DirtyCast/run/certs)
###################################################################################################

FROM dirtycast/runbase:active
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>


###################################################################################################
# Install stunnel
# Install kubectl
###################################################################################################

RUN apt-get install -y stunnel4 && \
    apt-get install -y kubectl


###################################################################################################
# Add the stunnel config file 
###################################################################################################

ADD stunnel.conf /home/gs-tools/stunnel/stunnel.conf


###################################################################################################
# Add the stunnel launching script to be invoked when running the container. 
#################################################################################################

ADD stunnel_launch.sh /home/gs-tools/stunnel/stunnel_launch.sh
RUN chmod ugo+x /home/gs-tools/stunnel/stunnel_launch.sh
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

WORKDIR /home/gs-tools/stunnel/

