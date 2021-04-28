FROM library/centos:7.4.1708

MAINTAINER dmikulec@ea.com

RUN echo "Installing kubectl..." && \
    curl -LO https://storage.googleapis.com/kubernetes-release/release/v1.11.6/bin/linux/amd64/kubectl && \
    chmod +x kubectl

COPY blaze-init.sh /

CMD ["./blaze-init.sh"]

