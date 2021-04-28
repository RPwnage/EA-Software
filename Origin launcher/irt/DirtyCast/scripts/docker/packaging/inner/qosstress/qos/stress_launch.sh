#!/bin/bash
# Bash script in charge of performing the final config steps and then firing the
# qostress process

# cleanup function: release DPA ports and delete sync file
cleanup() {
    echo "Initiating resource cleanup ..."

    # from here, ignore the EXIT pseudo-signal and the regular signals
    # we are already in the final clean up and there is no reason for executing that twice
    trap '' EXIT SIGINT SIGTERM

    #only when running one client do we manage the port and upload logs/cores to s3
    if [ $TOTAL_CLIENTS -eq 1 ]; then
    
        # if necessary, delete dynamically allocated ports
        if [ -z "$QOS_PROBE_PORT" ]; then
            curl -X DELETE localhost:11021/ports/$POD_NAME/QosProbePort
            if [ $? -eq 0 ]; then
                echo "Successfully cleaned up qos probe port" $QOS_PROBE_PORT
            else 
                echo "Failed to clean up qos probe port" $QOS_PROBE_PORT
            fi
        fi

        echo "Uploading to S3: app=qos-stress podname=$POD_NAME containerid=qosstress"
        /home/gs-tools/backupToS3.py -app=qos-stress -podname=$POD_NAME -containerid=qosstress \
            -coredir=/tmp -logdir=/home/gs-tools/qosstress/QOSBOT_Linux_Validation_0000 
    fi

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
: "${ENVIRONMENT?Cannot run without ENVIRONMENT env var being defined.}"
: "${QOS_HOSTING_REGION?Cannot run without QOS_HOSTING_REGION env var being defined.}"
: "${QOS_HOSTING_POOL?Cannot run without QOS_HOSTING_POOL env var being defined.}"
: "${QOS_COORDINATOR_ENDPOINT?Cannot run without QOS_COORDINATOR_ENDPOINT env var being defined.}"
: "${QOS_COORDINATOR_PORT?Cannot run without QOS_COORDINATOR_PORT env var being defined.}"
: "${TOTAL_CLIENTS?Cannot run without TOTAL_CLIENTS env var being defined.}"

# Find out the "probe" port that this container should bind to, if we are running many we use the index, if we are running 1 we use this "pas" type feature
if [ $TOTAL_CLIENTS -eq 1 ]; then
    if [ -z "$QOS_PROBE_PORT" ]; then
        QOS_PROBE_PORT=`curl -s localhost:11021/ports/"$POD_NAME"/QosProbePort?protocol=udp | grep -Po '(?<=PortNumber":)[0-9]+'`
        if [ -z "$QOS_PROBE_PORT" ]; then
            echo "Failed to get qos probe port"
            exit 1
        fi
    fi
fi
echo "qos probe port:" $QOS_PROBE_PORT

if [ -z "$METRICS_PORT" ]; then
    METRICS_PORT=8125
fi

if [ -z "$METRICS_ENDPOINT" ]; then
    METRICS_ENDPOINT=127.0.0.1
fi

if [ -z "$METRICS_RATE_GS" ]; then
    METRICS_RATE_GS=10000
fi

if [ -z "$LOG_VERBOSITY" ]; then
    LOG_VERBOSITY=1
fi

if [ -z "$LOGGER_PRUNE_TIME" ]; then
    LOGGER_PRUNE_TIME=30
fi

if [ -z "$LOGGER_DUAL" ]; then
    LOGGER_DUAL=1
fi

if [ -z "$USE_SSL" ]; then
    USE_SSL=1
fi

if [ -z "$QOS_STRESS_MAX_RATE" ]; then
    QOS_STRESS_MAX_RATE=10000
fi

if [ -z "$QOS_STRESS_PROFILE" ]; then
    QOS_STRESS_PROFILE="validate"
fi

if [ -z "$QOS_STRESS_HTTP_LINGER" ]; then
    QOS_STRESS_HTTP_LINGER=1
fi

# parse the pod name to get the index. for stateful sets this guarenteed but it can also be enforced by our old
# deployment type
[[ $POD_NAME =~ -([0-9]+)$ ]]
pod_index=${BASH_REMATCH[1]}

# change the start index if passed in, our current charts don't pass in start index, we leave this in for legacy use
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

# start the clients
for ((index = $start; index < $((start + TOTAL_CLIENTS)); index += 1))
do
    # calculate the bot index in the expected format
    bot_index=$(printf "%05d" $index)
    # format the log name
    log_file=$(printf "qosstress%05d.log" $index)
    
    if [ $TOTAL_CLIENTS -gt 1 ]; then
        QOS_PROBE_PORT=$bot_index
    fi

    # start the stress client
    ./$SELECTED_QOSSTRESS_BINARY ./validate.cfg \
    -qosprofile=$QOS_STRESS_PROFILE \
    -pingsite=$QOS_HOSTING_REGION \
    -environment=$ENVIRONMENT \
    -pool=$QOS_HOSTING_POOL \
    -deployinfo=$POD_NAME \
    -deployinfo2=$KUBERNETES_NAMESPACE \
    -maxrunrate=$QOS_STRESS_MAX_RATE \
    -qoscaddr=$QOS_COORDINATOR_ENDPOINT \
    -qoscport=$QOS_COORDINATOR_PORT \
    -qosprobeport=$QOS_PROBE_PORT \
    -usessl=$USE_SSL \
    -httpsolinger=$QOS_STRESS_HTTP_LINGER \
    -metricsaggregator.rate=$METRICS_RATE_GS \
    -metricsaggregator.addr=$METRICS_ENDPOINT \
    -metricsaggregator.port=$METRICS_PORT \
    -debuglevel=$LOG_VERBOSITY \
    -logger.dual=$LOGGER_DUAL \
    -logger.cfgfile=qosstress_logger.cfg \
    -logger.logfile=QOSBOT_Linux_Validation_0000/$log_file \
    -logger.prunetime=$LOGGER_PRUNE_TIME \
     $EXTRA_CONFIG &

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
echo "last qosstress process exited with code" $exit_code

# use last qosstress exit value as the exit value for the script
exit $exit_code

