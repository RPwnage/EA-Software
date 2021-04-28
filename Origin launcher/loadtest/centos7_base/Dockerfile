FROM library/centos:7.4.1708

# Currently, all TechOps puppet profiles and Chef recipes set up yum repositories specific to the CentOS 7 minor release.
# To ensure that container-built blazeservers will run outside containers on TechOps hardware, we must do the same.
ENV MINOR_VERSION .4

# The default yum repos can't simply be deleted, due to a quirk in Docker:
# (https://stackoverflow.com/questions/35962538/dockerfile-removing-a-file-in-one-run-command-it-is-still-present-in-the-next/35966336).
# However, we can overwrite them instead.
RUN echo "keepcache=0" >> /etc/yum.conf && \
    echo "tolerant=1" >> /etc/yum.conf && \
    for repo in $(ls /etc/yum.repos.d/); do \
        echo "" > /etc/yum.repos.d/${repo}; \
    done

RUN for repo in os epel esm ea-online-it updates extras; do \
        echo "[uxdepot-centos7${MINOR_VERSION}-${repo}]" >> /etc/yum.repos.d/uxdepot-centos7${MINOR_VERSION}-${repo}.repo && \
        echo "name = Uxdepot ${repo} Repository for CentOS 7${MINOR_VERSION}" >> /etc/yum.repos.d/uxdepot-centos7${MINOR_VERSION}-${repo}.repo && \
        echo "baseurl = http://uxdepot.abn-iad.ea.com/mrepo/centos7${MINOR_VERSION}-server-x86_64/RPMS.${repo}" >> /etc/yum.repos.d/uxdepot-centos7${MINOR_VERSION}-${repo}.repo && \
        echo "enabled = 1" >> /etc/yum.repos.d/uxdepot-centos7${MINOR_VERSION}-${repo}.repo && \
        echo "gpgcheck = 0" >> /etc/yum.repos.d/uxdepot-centos7${MINOR_VERSION}-${repo}.repo && \
        echo "skip_if_unavailable = 1" >> /etc/yum.repos.d/uxdepot-centos7${MINOR_VERSION}-${repo}.repo && \
        echo "sslverify = 0" >> /etc/yum.repos.d/uxdepot-centos7${MINOR_VERSION}-${repo}.repo; \
    done

# Install yum-plugin-ovl which is required for yum checksums to work in Docker: https://github.com/docker/docker/issues/10180
# Blaze requires libpcap and openssl to run
RUN yum -y update && \
    yum -y install yum-plugin-ovl && \
    yum -y install libpcap && \
    yum -y install OpenSSL-1.1.1d-1.x86_64 && \
    yum -y install MySQL-python && \
    yum clean all

# Set directory for ea openssl package
RUN echo "OPENSSL_ROOT_DIR=/opt/ea/openssl" > /etc/profile.d/openssl.sh && \
    echo "export OPENSSL_ROOT_DIR" >> /etc/profile.d/openssl.sh && \
    echo "PATH=\$PATH:\$OPENSSL_ROOT_DIR/bin" >> /etc/profile.d/openssl.sh && \
    echo "export PATH" >> /etc/profile.d/openssl.sh && \
    chmod +x /etc/profile.d/openssl.sh && \
    source /etc/profile.d/openssl.sh 

# Python
# The CentOS image we inherit from already contains a Python version that
# can be used with Blaze. But we also require the MySQL-python module (installed above).
# In addition to this, we need to make the Python binary available at path
# /opt/local/bin/python since some scripts related to Blaze expect it to
# be available there.
RUN ln -s /usr/bin/python /usr/local/bin/python && \
    mkdir -p /opt/local/bin && \
    ln -s /usr/bin/python /opt/local/bin/python
