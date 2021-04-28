#!/bin/bash
# Bash script in charge of performing the final config steps and then firing the
# gsrvstress process

# cleanup function: release DPA ports and delete sync file
cleanup() {
    echo "Initiating resource cleanup ..."

    # from here, ignore the EXIT pseudo-signal and the regular signals
    # we are already in the final clean up and there is no reason for executing that twice
    trap '' EXIT SIGINT SIGTERM
}

# regular signal handler
sig_handler() {
    echo "Executing signal handler for signal" $1
    local exit_code=0

    # from here, ignore the EXIT pseudo-signal because we are explicitly calling the handler
    # (with bash, the EXIT pseudo-signal is signaled as a result of exit being called in the regular signal handler)
    trap '' EXIT

    # if the process is running in the background, tell it to drain
    if [ -z $! ]; then
        echo "No need to drain the process because it has not yet been started"
        exit_code=0
    else
        # send drain signal to the process using its job identifier
        kill -s USR1 $!

        if [ $? -eq 0 ]; then
            echo "Successfully initiated the draining of the process running as background job" ${!}

            # we use wait to suspend execution until the child process exits.
            # wait is a function that always interrupts when a signal is caught
            wait $!

            # save exit code of the process
            exit_code=$?
        else
            echo "Failed to initiate draining of the process running as background job" ${!}
            exit_code=1
        fi
    fi

    # cleanup resources
    cleanup

    exit $exit_code
}

# register the regular signals handler
sigint_handler() {
    sig_handler "SIGINT"
}
sigterm_handler() {
    sig_handler "SIGTERM"
}
trap sigint_handler SIGINT
trap sigterm_handler SIGTERM

# exit_handler: invoked when bash EXIT pseudo-signal is fired
exit_handler() {
    echo "Executing EXIT handler with exit code" $exit_code

    # cleanup resources
    cleanup
}

# register the exit handler with the bash-specific EXIT (end-of-script) pseudo-signal
# the exit handler assumes the exit_code is defined
exit_code=0
trap exit_handler EXIT

echo "pid is $$"

printenv

# make sure required environment variables are specified
: "${KUBERNETES_NAMESPACE?Cannot run without KUBERNETES_NAMESPACE env var being defined.}"
: "${POD_NAME?Cannot run without POD_NAME env var being defined.}"
: "${NODE_NAME?Cannot run without NODE_NAME env var being defined.}"
: "${ENVIRONMENT?Cannot run without ENVIRONMENT env var being defined.}"
: "${BLAZE_SERVICE_NAME?Cannot run without BLAZE_SERVICE_NAME env var being defined.}"
: "${PACKET_SIZE?Cannot run without PACKET_SIZE env var being defined.}"
: "${MAX_GAME_SIZE?Cannot run without MAX_GAME_SIZE env var being defined.}"
: "${TOTAL_CLIENTS?Cannot run without TOTAL_CLIENTS env var being defined.}"

if [ -z "$METRICS_PORT" ]; then
    METRICS_PORT=8125
fi

if [ -z "$METRICS_ENDPOINT" ]; then
    METRICS_ENDPOINT=127.0.0.1
fi

if [ -z "$METRICS_RATE_GS" ]; then
    METRICS_RATE_GS=15000
fi

# parse the pod name to get the index. for stateful sets this guarenteed but it can also be enforced by our old
# deployment type
[[ $POD_NAME =~ -([0-9]+)$ ]]
pod_index=${BASH_REMATCH[1]}

# change the start index if passed in. our current charts don't pass in start index, we leave this in for legacy use
# cases
if [ -z $START_INDEX ]; then
    # if pod index cannot be parsed then we start at zero
    if [ -z $pod_index ]; then
        start=0
    # otherwise we calculate our start index based on the current pod, total clients and our offset
    else
        start=$(($TOTAL_CLIENTS * $pod_index + $START_OFFSET))
    fi
else
    start=$START_INDEX
fi

if [ -z "$SECONDS_BETWEEN_CLIENTS" ]; then
    SECONDS_BETWEEN_CLIENTS=0.2
fi

# get the public hostname from the k8s node metadata
public_hostname=$(kubectl get nodes $NODE_NAME -o=jsonpath='{.status.addresses[?(@.type=="ExternalDNS")].address}')
if [ $? -ne 0 ]; then
    echo "querying ExternalDNS from kubernetes cluster metadata failed, make sure your service account is properly configured."
    exit 1
elif [ -z "$public_hostname" ]; then
    # use the fallback when the ExternalDNS property is not available
    public_hostname=$(kubectl get nodes $NODE_NAME -o jsonpath='{.metadata.annotations.cloud\.ea/dns-name}')
fi

# start the clients
for ((index = $start; index < $((start + TOTAL_CLIENTS)); index += 1))
do
    # calculate the bot index in the expected format
    bot_index=$(printf "%05d" $index)
    # format the log name
    log_file=$(printf "gsrvstress%05d.log" $index)

    # start the stress client
    ./$SELECTED_GSRVSTRESS_BINARY stress.cfg $bot_index \
        -USE_STRESS_LOGIN=1 \
        -deployinfo=$POD_NAME \
        -deployinfo2="$KUBERNETES_NAMESPACE" \
        -hostname=$public_hostname \
        -SERVICE=$BLAZE_SERVICE_NAME \
        -ENVIRONMENT=$ENVIRONMENT \
        -GAME_MODE=$GAME_MODE \
        -PACKET_SIZE=$PACKET_SIZE \
        -MIN_PLAYERS=$MAX_GAME_SIZE \
        -MAX_PLAYERS=$MAX_GAME_SIZE \
        -VOIP_SUBPACKET_RATE=$VOIP_SUBPACKET_RATE \
        -VOIP_SUBPACKET_SIZE=$VOIP_SUBPACKET_SIZE \
        -metricsaggregator.addr=$METRICS_ENDPOINT \
        -metricsaggregator.port=$METRICS_PORT \
        -metricsaggregator.rate=$METRICS_RATE_GS \
        $EXTRA_CONFIG &> $log_file &

    # sleep for a bit
    sleep $SECONDS_BETWEEN_CLIENTS
done

# write a readiness probe file when we are done
echo "ready" >> readiness_probe.kube

# we use wait to suspend execution until the last child process exits.
# wait is a function that always interrupts when a signal is caught
# waits for all the children to quit
wait < <(jobs -p)

# delete the readiness probe file
rm readiness_probe.kube

# save last detected exit value
exit_code=$?
echo "last gsrvstress process exited with code" $exit_code

# use last gsrvstress exit value as the exit value for the script
exit $exit_code

