ARG BLAZE_BASE_IMAGE
FROM $BLAZE_BASE_IMAGE

ARG BLAZE_BASE_DIR
ADD archive/*.tar.gz $BLAZE_BASE_DIR

# jq is used to parse docker HTTP API output in blazeserver launch.sh script
RUN apt-get -y install gdb jq
COPY launch.sh /
