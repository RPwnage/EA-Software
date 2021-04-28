#!/bin/bash
# bash script creating the final lightweight voipserver container by invoking docker build from within a docker container

# Find out what app (gameserver or voipserver) needs to be containerized
if [ -z $1 ]; then
    echo "1st argument missing."
    exit 1
else
    if [[ ($1 == "voipserver") || ($1 == "gameserver") || ($1 == "qosstress") || ($1 == "gsrvstress") ]]; then
         app_arg=$1
    else
         echo "Invalid 1st argument. Valid values for 1st argument: voipserver, gameserver, qosstress, gsrvstress"
         exit 1
    fi
fi
echo "app arg:" $app_arg

# Find out what build (debug or final) needs to be containerized
if [ -z $2 ]; then
    echo "2nd argument missing"
    exit 1
else
    if [[ ($2 == "debug") || ($2 == "final") ]]; then
         build_arg=$2
    else
         echo "Invalid 2nd argument. Valid values for 2nd argument: debug, final"
         exit 1
    fi
fi
echo "build arg:" $build_arg

# Find out what operationg mode (otp or cc or qos) needs to be containerized
if [ -z $3 ]; then
    echo "3rd argument missing"
    exit 1
else
    if [[ ($3 == "otp") || ($3 == "cc") || ($3 == "qos") ]]; then
         mode_arg=$3
    else
         echo "Invalid 3rd argument. Valid values for 3rd argument: otp, cc, qos"
         exit 1
    fi
fi
echo "mode arg:" $mode_arg

# Find out what version to assign to the container
if [ -z $4 ]; then
    version_arg="latest"
else
    version_arg=$4
fi

# Find out which repository to push to
if [ -z $5 ]; then
    repo="dirtycast"
else
    repo=$5
fi

# Find out if we have a runbase override
if [ -z $RUNBASE ]; then
    runbase=
else
    runbase="--build-arg RUNBASE_IMAGE=$RUNBASE"
fi

# Set container's name
if [ $build_arg == "debug" ]; then
    build_ext="d"
else
    build_ext="f"
fi
container_name="$app_arg$build_ext""_""$mode_arg"

# Move to the docker context matching the input arguments
cd /tmp/EAGS/inner/$app_arg/$mode_arg

# From the snapshot saved in the current container, recreate the DirtyCast run directories in the docker context for the "inner docker build"
mkdir ./DirtyCast
cp -r /tmp/EAGS/buildoutput/run ./DirtyCast
mkdir -p ./DirtyCast/GameServer/run
cp -r /tmp/EAGS/buildoutput/GameServer/run ./DirtyCast/GameServer
mkdir -p ./DirtyCast/qosstress/run
cp -r /tmp/EAGS/buildoutput/qosstress/run ./DirtyCast/qosstress
mkdir -p ./DirtyCast/stress/run
cp -r /tmp/EAGS/buildoutput/stress/run ./DirtyCast/stress

# Invoke docker in docker
docker build --build-arg DIRTYCAST_BINARY_ARG=$app_arg$build_ext $runbase -t $repo/$container_name:$version_arg .
if [ $? -eq 0 ]; then
    echo "Congrats!"
    echo "You have just created a new container image with name" $container_name "in the" $repo "repository. It is ready for deployment!"
    echo "Use the -docker images- command to see it in the list of images known by your docker host."
    echo "Notice that it is not much bigger than the dirtycast/runbase image, and much smaller than the dirtycast/packaging image."
else 
    echo "FAILED to create" $container_name "in the" $repo "repository."
fi
