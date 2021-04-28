FROM library/ubuntu:bionic-20190912.1

# Currently, all TechOps puppet profiles and Chef recipes set up yum repositories specific to the CentOS 7 minor release.
# To ensure that container-built blazeservers will run outside containers on TechOps hardware, we must do the same.
ENV MINOR_VERSION .4

# Blaze requires libpcap and openssl to run
RUN apt-get update && \
    apt-get install -y libpcap0.8 libssl1.1 libcurl4 curl python-mysqldb gdb

# Python
# The CentOS image we inherit from already contains a Python version that
# can be used with Blaze. But we also require the MySQL-python module (installed above).
# In addition to this, we need to make the Python binary available at path
# /opt/local/bin/python since some scripts related to Blaze expect it to
# be available there.
RUN ln -s /usr/bin/python /usr/local/bin/python && \
    mkdir -p /opt/local/bin && \
    ln -s /usr/bin/python /opt/local/bin/python
