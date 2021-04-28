#!/bin/bash

# generate the version file at ${package.builddir}/pregen_version which is used in the build to generate the final version.c (for more
# information see mkver.cs)
./nant mkver -D:dirtycast-pregen=true

# build dirtycast within docker
docker run ${@} -it dirtycast/build:active ./build.sh

# remove the generate file to prevent it using old version in non-docker builds
./nant clean_pregen
