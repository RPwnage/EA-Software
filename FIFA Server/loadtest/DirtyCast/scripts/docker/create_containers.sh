#!/bin/bash

###
### create_containers — Create, and push DirtyCast images to be deployed on Cloud Fabric
###
### Usage:
###   create_containers <path to DirtyCast dir> <tag> <email> [-s] [-r]
###
### Options:
###   <Path to DirtyCast>   Path to P4 DirtyCast folder location
###   <tag>       Used to tag continers
###   <email>     Your email address
###   -c          Remove successfully pushed images
###   -p          Push images to repository
###   -r          Specify docker repo
###                     default     -r dirtycast
###                     e.g.        -r docker-blaze.darkside.ea.com:443
###   -s          Create only the Stress Client containers
###   -u          username for logging into package server
###   -P          password for logging into package server
###   -h          Show this message.
###

help() {
    sed -rn 's/^### ?//;T;p' "$0"
}

push_image() {
    docker push $1
    if [ $? -ne 0 ]; then
        echo "Failed to push" $1
    fi
}

# Default Values
# $$TODO$$ flip these boolean options as checking for zero seems backwards
stress=1
clean=1
dockerPush=1
repository=dirtycast

if [[ $# -lt "3" ]]; then
    help
    exit 1
fi

pathToDirtyCast=$1
versionTag=$2
maintainer=$3
shift 3

while getopts cpr:u:P:s flag
do
    case "${flag}" in
        c)
            clean=0
        ;;
        p)
            dockerPush=0
        ;;
        r)
            repository=${OPTARG}
        ;;
        s)
            stress=0
        ;;
        u)
            pkguser=${OPTARG}
        ;;
        P)
            pkgpass=${OPTARG}
        ;;
        h)
            help
            exit 1
        ;;
    esac
done

# set the build to be interactive if username and password are not set
if [[ ( -z "$pkguser" ) && ( -z "$pkgpass" ) ]]; then
    extra_args="-it"
else
    extra_args=$(printf " -t -e PKG_USER=%s -e PKG_PASSWORD=%s" ${pkguser} ${pkgpass})
fi

if [ ${dockerPush} -eq 0  ]; then
    if [ "${repository}" == "dirtycast" ]; then
        dockerPush=1
        echo "Cannot push to default repository; use -r to change"
    fi
fi

# make sure we are running from the dirtycast folder
cd ${pathToDirtyCast}/DirtyCast

# Create the BASE container
docker build --rm=true -t dirtycast/base:active scripts/docker/base
if [ $? -ne 0 ]; then
    echo "Failed to create BUILDBASE container..."
    exit 1
fi

# Create the BUILD container
docker build --rm=true -t dirtycast/build:active scripts/docker/build
if [ $? -ne 0 ]; then
    echo "Failed to create BUILD container..."
    exit 1
fi

# generate the version file at ${package.builddir}/pregen_version which is used in the build to generate the final version.c (for more
# information see mkver.cs)
./nant mkver -D:dirtycast-pregen=true

# Run the BUILD container
if [ ${stress} -eq 0 ]; then
    docker run ${extra_args} --name dirtycast_build_container -v ${pathToDirtyCast}:/tmp/EAGS/gos-stream/ml dirtycast/build:active ./build-stress.sh
else
    docker run ${extra_args} --name dirtycast_build_container -v ${pathToDirtyCast}:/tmp/EAGS/gos-stream/ml dirtycast/build:active ./build.sh
fi

if [ $? -ne 0 ]; then
    echo "Failed to successfully build the dirtycast code..."
    docker rm dirtycast_build_container
    exit 1
fi
docker commit -a ${maintainer} dirtycast_build_container dirtycast/buildoutput:active
docker rm dirtycast_build_container

# Verify that buildoutput folder exists before continuing
docker run -t --rm=true --name dirtycast_buildoutput_container dirtycast/buildoutput:active ls buildoutput
if [ $? -ne 0 ]; then
    echo "Failed to find buildoutput folder..."
    docker rmi dirtycast/buildoutput:active
    exit 1
fi

# Create the RUNBASE container
docker build --rm=true -t dirtycast/runbase:active scripts/docker/runbase
if [ $? -ne 0 ]; then
    echo "Failed to create RUNBASE container..."
    exit 1
fi

# Create the PACKAGING container
docker build --rm=true -t dirtycast/packaging:active scripts/docker/packaging
if [ $? -ne 0 ]; then
    echo "Failed to create PACKAGING container..."
    exit 1
fi

# Create the voip server and game server containers
if [ ${stress} -eq 0 ]; then 
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active gsrvstress debug cc ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active gsrvstress final cc ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active gsrvstress debug otp ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active gsrvstress final otp ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active qosstress debug qos ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active qosstress final qos ${versionTag} ${repository}
else
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active voipserver debug cc ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active voipserver final cc ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active voipserver debug otp ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active voipserver final otp ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active voipserver debug qos ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active voipserver final qos ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active gameserver debug otp ${versionTag} ${repository}
    docker run --rm -t --name dirtycast_packaging_container -v /var/run/docker.sock:/var/run/docker.sock dirtycast/packaging:active gameserver final otp ${versionTag} ${repository}

    # Create the stunnel container
    docker build --rm=true -t ${repository}/stunnel:${versionTag} scripts/docker/stunnel
fi

echo "Successfully created ${versionTag} DirtyCast containers"
docker images

# if the option is set, push the images
if [ ${dockerPush} -eq 0 ]; then
    if [ ${stress} -eq 0 ]; then
        push_image ${repository}/gsrvstressd_cc:${versionTag}
        push_image ${repository}/gsrvstressf_cc:${versionTag}
        push_image ${repository}/gsrvstressd_otp:${versionTag}
        push_image ${repository}/gsrvstressf_otp:${versionTag}
        push_image ${repository}/qosstressd_qos:${versionTag}
        push_image ${repository}/qosstressf_qos:${versionTag}
    else
        push_image ${repository}/voipserverd_cc:${versionTag}
        push_image ${repository}/voipserverf_cc:${versionTag}
        push_image ${repository}/voipserverd_otp:${versionTag}
        push_image ${repository}/voipserverf_otp:${versionTag}
        push_image ${repository}/voipserverd_qos:${versionTag}
        push_image ${repository}/voipserverf_qos:${versionTag}
        push_image ${repository}/gameserverd_otp:${versionTag}
        push_image ${repository}/gameserverf_otp:${versionTag}
        push_image ${repository}/stunnel:${versionTag}
    fi
fi

# if the option is set, clean the images
if [ ${clean} -eq 0 ]; then
    if [ ${stress} -eq 0 ]; then
        docker rmi ${repository}/gsrvstressd_cc:${versionTag}
        docker rmi ${repository}/gsrvstressf_cc:${versionTag}
        docker rmi ${repository}/gsrvstressd_otp:${versionTag}
        docker rmi ${repository}/gsrvstressf_otp:${versionTag}
        docker rmi ${repository}/qosstressd_qos:${versionTag}
        docker rmi ${repository}/qosstressf_qos:${versionTag}
    else
        docker rmi ${repository}/voipserverd_cc:${versionTag}
        docker rmi ${repository}/voipserverf_cc:${versionTag}
        docker rmi ${repository}/voipserverd_otp:${versionTag}
        docker rmi ${repository}/voipserverf_otp:${versionTag}
        docker rmi ${repository}/voipserverd_qos:${versionTag}
        docker rmi ${repository}/voipserverf_qos:${versionTag}
        docker rmi ${repository}/gameserverd_otp:${versionTag}
        docker rmi ${repository}/gameserverf_otp:${versionTag}
        docker rmi ${repository}/stunnel:${versionTag}
    fi
    docker rmi dirtycast/runbase:active
    docker rmi dirtycast/packaging:active
    docker rmi dirtycast/buildoutput:active
    docker rmi dirtycast/build:active
    docker rmi dirtycast/base:active
fi
