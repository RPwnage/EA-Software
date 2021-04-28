###################################################################################################
# Author:  mclouatre
# Purpose: Creates an image that can later be instantiated to run the steps required to package
#          one of the dirtycast binaries (gameserver or voipserver; dbg or opt) into another container
#          image with minimal foot print and ready for deployment. This container internally exercises
#          the docker client. This image consists of dirtycast/buildoutput + local instrumentation to
#          perform docker-in-docker for creating the final specialized dirtycast container.
# Usage:   cd <...>\DirtyCast\scripts\docker\packaging
#          docker build -t dirtycast/packaging:active .
#          docker run --name <container name> -v /var/run/docker.sock:/var/run/docker.sock -it dirtycast/packaging:active <app arg: voipserver|gameserver> <build arg: debug|final> <mode arg: cc|otp|qos> {optional container's tag ("latest" is used if not specified)}
# Notes:   "-v /var/run/docker.sock:/var/run/docker.sock" is required for this container to have
#          access to the Docker socket (the unix socket that the docker host is listening on) -
#          required to allow the container to internally build the "inner" Dockerfile used to
#          create the final minimal footprint deployment container.
# Output:  A new container image under the "dirtycast" repository in the list
#          of images known by the docker host. (use "docker images" to see that list). Depending 
#          on the argument passed to the "docker run" command, it will have one of these names:
#               <app arg>   <build arg> <mode arg>  ---->   <name>
#               voipserver  debug       cc          ---->   voipserverd_cc
#               voipserver  final       cc          ---->   voipserverf_cc
#               voipserver  debug       otp         ---->   voipserverd_otp
#               voipserver  final       otp         ---->   voipserverf_otp
#               voipserver  debug       qos         ---->   voipserverd_qos
#               voipserver  final       qos         ---->   voipserverf_qos
#               gameserver  debug       otp         ---->   gameserverd
#               gameserver  final       otp         ---->   gameserverf
#               gsrvstress  debug       cc          ---->   gsrvstressd
#               gsrvstress  final       cc          ---->   gsrvstressf
#               gsrvstress  debug       otp         ---->   gsrvstressd
#               gsrvstress  final       otp         ---->   gsrvstressf
#               qosstress   debug       qos         ---->   qosstressd
#               qosstress   final       qos         ---->   qosstressf
#
#          Usage with Docker ToolBox VirtualBox VM:
#               docker run --name <container name> --net=host -it dirtycast/<container image name>:latest
###################################################################################################

FROM dirtycast/buildoutput:active
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>


###################################################################################################
# Add the files used to generate the final DirtyCast container to be deployed in the cloud.
# That container image needs to be minimal footprint, i.e CentOS base image + hosting server binary.
#     * /tmp/EAGS/Dockerfile     --> inner Dockerfile used when invoking "docker build" internally
#     * /tmp/EAGS/.ignoredocker  --> used to limit the "context" sent by "docker build" to the docker host
#     * /tmp/EAGS/voipserver_launch.sh --> script to be invoked when running the resulting container
# 
# Note: The inner dockerfile has the "USER gos-tools" command to make sure stuff in the final deployment
#       container does not run as "root".
###################################################################################################

ADD ./inner /tmp/EAGS/inner


###################################################################################################
# Add the bash script used as the container's entry point
###################################################################################################

ADD dockerindocker.sh /tmp/EAGS/dockerindocker.sh


###################################################################################################
# Set container's working directory and entry point
###################################################################################################

USER root
WORKDIR /tmp/EAGS
ENTRYPOINT ["/tmp/EAGS/dockerindocker.sh"]
