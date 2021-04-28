FROM src.ent.ea.com:8443/gs/gs_docker_sandbox/blaze-centos7-base:1.2

ADD linux64/nodejs.tgz /mocksrv/config/
COPY common /mocksrv/common
COPY bitc /mocksrv/config
COPY mockexternalservicexone/*.js /mocksrv/config/
COPY mockexternalserviceps4/*.js /mocksrv/config/
COPY mockexternalserviceps5/*.js /mocksrv/config/
RUN mv /mocksrv/config/psnserver.js /mocksrv/config/ps4server.js && \
    mv /mocksrv/config/mpsdserver.js /mocksrv/config/xoneserver.js && \
    mv /mocksrv/config/psnserverps5.js /mocksrv/config/ps5server.js