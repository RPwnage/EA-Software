###################################################################################################
# Author:  eesponda
# Purpose: Creates an image that can later be used as a basis to run dirtycast (stress) processes.
#          Adds additional utilities ontop of our runbase
# Usage:   cd <...>\DirtyCast\scripts\docker\runbase\stress
#          docker build --rm=true -t dirtycast/stressbase:active .
# Output:  A new container image called "stressbase" under the "dirtycast" repository in the list
#          of images known by the docker host. (use "docker images" to see it)
###################################################################################################

FROM dirtycast/runbase:active
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>


###################################################################################################
# Install stress tools and dependencies
#   * linux-tools            --> installs performance measuring tool for us to profiling our server
###################################################################################################

RUN apt-get -y install linux-tools-generic linux-tools-common linux-tools-`uname -r`
