###################################################################################################
# Author:  mclouatre
# Purpose: Creates a debug image equiped with network tools (gdb, ss) and a user with the
#          right privileges to use them. When network-related info is required to support an
#          investigation in production, this container can be deployed on a specific box and used
#          to dump socket statistics or to perform a packet capture.
#          Consists of a CentOS 7.4 base + some user/group creation + installations of tools.
#
# Usage:   cd <...>\DirtyCast\scripts\docker\net-tools
#          docker build --rm=true -t debugging/net-tools:active .
#
# Output:  A new container image called "net-tools" under the "debugging" repository in the list
#          of images known by the docker host. (use "docker images" to see it)
#
#          Usage with Docker ToolBox VirtualBox VM:
#               docker run --name <container name> -it debugging/net-tools:actvie /bin/bash
#
# Note:    In CentOS 7, a lot of the command-line utilities for configuring and troubleshooting
#          network properties like arp, ifconfig, iptunnel, iwconfig, nameif, netstat, route -
#          are no longer there. They are still included in many Linux Distributions, (and you may be 
#          tempted to run - ‘yum install net-tools’ - to get them back into your Linux footprint). 
#          But they are considered deprecated (some security vulnerabilities may not get adequately
#          patched) and therefore should be phased out in favor of more modern replacements.
#          CentOS 7 team has decided to move to iproute2 package. 
# 
#               Net-tool old command            IPRoute2 new command
#               --------------------            --------------------
#               netstat                         ss, ip route, ip -s link, ip maddr
#               ifconfig                        ip addr, ip link, ip -s
#               etc.
###################################################################################################

FROM library/centos:7.4.1708
MAINTAINER Dirtysock <GS-DirtySockInternal@ea.com>


###################################################################################################
# Install tools
#   * which         --> "which" cmd
#   * iproute       --> collection of programs (ss, ip)
#   * tcpdump       --> tcpdump for packet captures
###################################################################################################

RUN yum -y install which && \
    yum -y install iproute && \
    yum -y install tcpdump


###################################################################################################
# Install sudo (required for using tcpdump/ss as gs-tools when "execing" into the container)
###################################################################################################

RUN yum -y install sudo


###################################################################################################
# Create the gs-tools user and gs-tools group
###################################################################################################

RUN groupadd -r gs-tools -g 627 && \
    useradd -u 627 -m -r -g gs-tools -d /home/gs-tools -s /sbin/nologin gs-tools && \
    chmod 755 /home/gs-tools


###################################################################################################
# Add the right file under /etc/sudoers.d to allow gs-tools to use ss and ip with elevated privileges
###################################################################################################

RUN echo "gs-tools ALL=(ALL) NOPASSWD:/usr/sbin/ss,/usr/sbin/ip,/usr/sbin/tcpdump" >> /etc/sudoers.d/0001_gs-tools


###################################################################################################
# Syntax check on the newly created file. Force docker build to exit if check fails
###################################################################################################

RUN visudo -cf /etc/sudoers.d/0001_gs-tools


###################################################################################################
# Set the user
###################################################################################################

USER gs-tools

