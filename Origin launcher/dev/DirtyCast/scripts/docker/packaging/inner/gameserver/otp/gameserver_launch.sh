#!/bin/bash
# Bash script in charge of performing the final config steps and then firing the
# gameserver process

# cleanup function: release DPA ports and delete sync file
cleanup() {
    echo "Initiating resource cleanup ..."

    # from here, ignore the EXIT pseudo-signal and the regular signals
    # we are already in the final clean up and there is no reason for executing that twice
    trap '' EXIT SIGINT SIGTERM

    if [ -z "$DIAGNOSTIC_PORT_GS" ]; then
        curl -X DELETE localhost:11021/ports/$POD_NAME/DiagnosticPortGS$INDEX
        if [ $? -eq 0 ]; then
            echo "Successfully cleaned up diagnostic port" $diagnostic_port
        else
            echo "Failed to clean up diagnostic port" $diagnostic_port
        fi
    fi

    echo "Uploading to S3: app=otp-game-server podname=$POD_NAME containerid=gameserver$INDEX"
    /home/gs-tools/backupToS3.py -app=otp-game-server -podname=$POD_NAME -containerid=gameserver$INDEX \
        -coredir=/tmp -logdir=/home/gs-tools/gameserver/$LOG_DIR
}

# regular signal handler
sig_handler() {
    echo "Executing signal handler for signal" $1
    local exit_code=0

    # from here, ignore the EXIT pseudo-signal because we are explicitly calling the handler
    # (with bash, the EXIT pseudo-signal is signaled as a result of exit being called in the regular signal handler)
    trap '' EXIT

    # if the gameserver process is running in the background, tell it to drain
    if [ -z $! ]; then
        echo "No need to drain the gameserver process because it has not yet been started"
        exit_code=0
    else
        # send drain signal to the gameserver using its job identifier
        kill -s USR1 $!

        if [ $? -eq 0 ]; then
            echo "Successfully initiated the draining of the gameserver process running as background job" ${!}

            # we use wait to suspend execution until the child process exits.
            # wait is a function that always interrupts when a signal is caught
            wait $!

            # save exit code of the gameserver process
            exit_code=$?
        else
            echo "Failed to initiate draining of the gameserver process running as background job" ${!}
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

    if [ $exit_code -ne 0 ]; then
        # report to OIv2 agent that this gameserver container is going away as a result of the gameserver exiting abnormally
        echo "gameserver.events.gameservercrash:1|g|#env:$ENVIRONMENT,site:$REGION,deployinfo:$POD_NAME,deployinfo2:$KUBERNETES_NAMESPACE,port:$diagnostic_port,index:$INDEX" > /dev/udp/$METRICS_ENDPOINT/$METRICS_PORT
    fi
}

# register the exit handler with the bash-specific EXIT (end-of-script) pseudo-signal
# the exit handler assumes the exit_code is defined
exit_code=0
trap exit_handler EXIT

echo "pid is $$"

printenv

# Make sure required environment variables are specified
: "${REGION?Cannot run without REGION env var being defined.}"
: "${ENVIRONMENT?Cannot run without ENVIRONMENT env var being defined.}"
: "${SHARED_FOLDER?Cannot run without SHARED_FOLDER env var being defined.}"
: "${KUBERNETES_NAMESPACE?Cannot run without KUBERNETES_NAMESPACE env var being defined.}"
: "${POD_NAME?Cannot run without POD_NAME env var being defined.}"
: "${BLAZE_SERVICE_NAME?Cannot run without BLAZE_SERVICE_NAME env var being defined.}"
: "${INDEX?Cannot run without INDEX env var being defined.}"
: "${MAX_GAME_SIZE?Cannot run without MAX_GAME_SIZE env var being defined.}"
: "${REDUNDANCY_LIMIT?Cannot run without REDUNDANCY_LIMIT env var being defined.}"
: "${LINK_STREAM?Cannot run without LINK_STREAM env var being defined.}"
: "${METRICS_ENDPOINT?Cannot run without METRICS_ENDPOINT env var being defined.}"
: "${METRICS_PORT?Cannot run without METRICS_PORT env var being defined.}"

# Find out the "diagnostic" port that this container should bind to
if [ -z "$DIAGNOSTIC_PORT_GS" ]; then
    diagnostic_port=`curl -s localhost:11021/ports/$POD_NAME/DiagnosticPortGS$INDEX?protocol=tcp | grep -Po '(?<=PortNumber":)[0-9]+'`
    if [ -z "$diagnostic_port" ]; then
        echo "Failed to get diagnostic port"
        exit 1
    fi
else
    diagnostic_port=$DIAGNOSTIC_PORT_GS
fi

echo "diagnostic port:" $diagnostic_port

echo "retrieving voipserver-gameserver communication port"
while [ ! -f $SHARED_FOLDER/$POD_NAME.syncfile ]
do
  echo "waiting for voipserver to create the sync file" $SHARED_FOLDER/$POD_NAME.syncfile "..."
  sleep 5
done

# read the voipserver's gameserver_port
# expected line format in the file:  "gameserver_port = XXXXX"
gameserver_port=`cat $SHARED_FOLDER/$POD_NAME.syncfile | awk '{print $3}'`
echo "found port" $gameserver_port "in sync file"

# start the gameserver as a background process for the bash script to be able to catch signals from docker host
./$SELECTED_GAMESERVER_BINARY ./gameserver.cfg \
    -diagnosticport=$diagnostic_port \
    -voipserverport=$gameserver_port \
    -pingsite=$REGION \
    -lobbyname=$BLAZE_SERVICE_NAME \
    -environment=$ENVIRONMENT \
    -maxclients=$MAX_GAME_SIZE \
    -linkstream=$LINK_STREAM \
    -redundancylimit=$REDUNDANCY_LIMIT \
    -certfilename=$NUCLEUS_CERT \
    -keyfilename=$NUCLEUS_KEY \
    -metricsaggregator.addr=$METRICS_ENDPOINT \
    -metricsaggregator.port=$METRICS_PORT \
    -metricsaggregator.rate=1000 \
    -metricsaggregator.maxaccumulationcycles=8 \
    $LOGGER_CONFIG \
    $EXTRA_CONFIG &

# we use wait to suspend execution until the child process exits.
# wait is a function that always interrupts when a signal is caught
wait $!

# save gamesserver exit value
exit_code=$?
echo "The gameserver process exited with code" $exit_code

# use gameserver exit value as the exit value for the script
exit $exit_code
