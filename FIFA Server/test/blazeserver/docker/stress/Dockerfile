FROM 284499763740.dkr.ecr.eu-west-1.amazonaws.com/gs-blazeclient-base:1.0
 
COPY archive /stress/
RUN cp -f /stress/bin/libmariadb.so /stress/bin/libmariadb.so.3
 
ENV STRESS_TOOL=stress \
    STRESS_BIN_PATH=/gs/bin/blazeclient \
    STRESS_CWD=/stress/bin \
    S3_BUCKET=bitc-ilt-stress-cores

WORKDIR /stress/bin
