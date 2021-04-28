ï»¿ARG BLAZE_BASE_IMAGE
FROM $BLAZE_BASE_IMAGE

ENV LD_LIBRARY_PATH=/app/lib PATH=/app/bin:$PATH

RUN mkdir /app

ARG BLAZE_TAR_FILE
ADD archive/$BLAZE_TAR_FILE /app/

LABEL "blaze.min-blaze-template-version"="v1.0.0"