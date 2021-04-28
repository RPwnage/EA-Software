# We base the container on Alpine Linux, a very minimal (~5MB) image based on BusyBox
FROM alpine:3.5

# Install some shell utils
# Add '--update' to the first 'apk add' to fetch the current package index
# (Since the package index can go stale fairly quickly, the Alpine image is not shipped with it)
RUN apk add --update bash bash-completion util-linux coreutils findutils grep sed

# Install redis
# To ensure Blaze can connect to it, we need to comment out the binding to IP 127.0.0.1 and disable protected mode in the configuration.
# We also disable the tcp keepalive (the blazeserver has its own keepalive logic for redis connections) and turn off daemonized mode.
# (Normally daemonized mode is off by default, but the Alpine Linux redis package turns it on.)
RUN apk add redis && \
    sed -Ei 's/^bind 127.0.0.1/# &/' /etc/redis.conf && \
    sed -Ei 's/^protected-mode yes/protected-mode no/' /etc/redis.conf && \
    sed -Ei 's/^tcp-keepalive 300/tcp-keepalive 0/' /etc/redis.conf && \
    sed -Ei 's/^daemonize yes/# &/' /etc/redis.conf

# curl and jq are used in blazeserver startredis.sh script
RUN apk add curl jq
