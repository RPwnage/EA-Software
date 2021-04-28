#!/bin/bash
# Bash script in charge of performing the final config steps and then firing the
# voipserver process

# cleanup function: release DPA ports and delete sync file
cleanup() {
    echo "Initiating resource cleanup ..."

    # from here, ignore the EXIT pseudo-signal and the regular signals
    # we are already in the final clean up and there is no reason for executing that twice
    trap '' EXIT SIGINT SIGTERM

    # if necessary, delete dynamically allocated ports
    if [ -z "$TUNNEL_PORT" ]; then
        curl -X DELETE localhost:11021/ports/$POD_NAME/TunnelPort
        if [ $? -eq 0 ]; then
            echo "Successfully cleaned up tunnel port" $tunnel_port
        else 
            echo "Failed to clean up tunnel port" $tunnel_port
        fi
    fi

    if [ -z "$DIAGNOSTIC_PORT" ]; then
        curl -X DELETE localhost:11021/ports/$POD_NAME/DiagnosticPort
                if [ $? -eq 0 ]; then
            echo "Successfully cleaned up diagnostic port" $diagnostic_port
        else 
            echo "Failed to clean up diagnostic port" $diagnostic_port
        fi
    fi

    if [ -z "$GAMESERVER_PORT" ]; then
        curl -X DELETE localhost:11021/ports/$POD_NAME/GameServerPort
                if [ $? -eq 0 ]; then
            echo "Successfully cleaned up gameserver port" $gameserver_port
        else 
            echo "Failed to clean up gameserver port" $gameserver_port
        fi
    fi

    # delete the sync file if it exists
    rm $SHARED_FOLDER/$POD_NAME.syncfile
    if [ $? -eq 0 ]; then
        echo "Sync file deleted"
    else
        echo "Failed to delete sync file"
    fi

    echo "Uploading to S3: app=otp-game-server podname=$POD_NAME containerid=voipserver"
    /home/gs-tools/backupToS3.py -app=otp-game-server -podname=$POD_NAME -containerid=voipserver \
        -coredir=/tmp -logdir=/home/gs-tools/voipserver/$LOG_DIR
}

# regular signal handler
sig_handler() {
    echo "Executing signal handler for signal" $1
    local exit_code=0

    # from here, ignore the EXIT pseudo-signal because we are explicitly calling the handler
    # (with bash, the EXIT pseudo-signal is signaled as a result of exit being called in the regular signal handler)
    trap '' EXIT

    # if the voipserver process is running in the background, tell it to drain
    if [ -z $! ]; then
        echo "No need to drain the voipserver process because it has not yet been started"
        exit_code=0
    else
        # send drain signal to the voipserver using its job identifier
        kill -s USR1 $!

        if [ $? -eq 0 ]; then
            echo "Successfully initiated the draining of the voipserver process running as background job" ${!}

            # we use wait to suspend execution until the child process exits.
            # wait is a function that always interrupts when a signal is caught
            wait $!

            # save exit code of the voipserver process
            exit_code=$?
        else 
            echo "Failed to initiate draining of the voipserver process running as background job" ${!}
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
        # report to OIv2 agent that this pod is going away as a result of the voipserver exiting abnormally
        echo "voipmetrics.events.voipservercrash:1|g|#mode:gameserver,env:$ENVIRONMENT,site:$REGION,deployinfo:$POD_NAME,deployinfo2:$KUBERNETES_NAMESPACE,port:$diagnostic_port" > /dev/udp/$METRICS_ENDPOINT/$METRICS_PORT
    fi

    # tell infrastructure to kill the pod
    kubectl delete pod $POD_NAME --wait=false
    if [ $? -eq 0 ]; then
        echo "voipserver container successfully initiated destruction of kub pod" $POD_NAME
    else 
        echo "voipserver container failed to initiate destruction of kub pod" $POD_NAME
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
: "${SHARED_FOLDER?Cannot run without SHARED_FOLDER env var being defined.}"
: "${KUBERNETES_NAMESPACE?Cannot run without KUBERNETES_NAMESPACE env var being defined.}"
: "${POD_NAME?Cannot run without POD_NAME env var being defined.}"
: "${NODE_NAME?Cannot run without NODE_NAME env var being defined.}"
: "${ENVIRONMENT?Cannot run without ENVIRONMENT env var being defined.}"
: "${NUCLEUS_CERT?Cannot run without NUCLEUS_CERT env var being defined.}"
: "${NUCLEUS_KEY?Cannot run without NUCLEUS_KEY env var being defined.}"
: "${MAX_GAME_SIZE?Cannot run without MAX_GAME_SIZE env var being defined.}"
: "${MAX_GAMES?Cannot run without MAX_GAMES env var being defined.}"
: "${METRICS_ENDPOINT?Cannot run without METRICS_ENDPOINT env var being defined.}"
: "${METRICS_PORT?Cannot run without METRICS_PORT env var being defined.}"

# Find out the "tunnel" port that this container should bind to
if [ -z "$TUNNEL_PORT" ]; then
    tunnel_port=`curl -s localhost:11021/ports/"$POD_NAME"/TunnelPort?protocol=udp | grep -Po '(?<=PortNumber":)[0-9]+'`
    if [ -z "$tunnel_port" ]; then
        echo "Failed to get tunnel port"
        exit 1
    fi
else
    tunnel_port=$TUNNEL_PORT
fi
echo "tunnel port:" $tunnel_port

# Find out the "diagnostic" port that this container should bind to
if [ -z "$DIAGNOSTIC_PORT" ]; then
    diagnostic_port=`curl -s localhost:11021/ports/"$POD_NAME"/DiagnosticPort?protocol=tcp | grep -Po '(?<=PortNumber":)[0-9]+'`
    if [ -z "$diagnostic_port" ]; then
        echo "Failed to get diagnostic port"
        exit 1
    fi
else
    diagnostic_port=$DIAGNOSTIC_PORT
fi
echo "diagnostic port:" $diagnostic_port

# Find out the "gameserver" port that this container should bind to
if [ -z "$GAMESERVER_PORT" ]; then
    gameserver_port=`curl -s localhost:11021/ports/"$POD_NAME"/GameServerPort?protocol=tcp | grep -Po '(?<=PortNumber":)[0-9]+'`
    if [ -z "$gameserver_port" ]; then
        echo "Failed to get gameserver port"
        exit 1
    fi
else
    gameserver_port=$GAMESERVER_PORT
fi
echo "gameserver port:" $gameserver_port

# get the public IP from the k8s node metadata
public_ip=$(kubectl get nodes $NODE_NAME -o=jsonpath='{.status.addresses[?(@.type=="ExternalIP")].address}')
if [ $? -ne 0 ]; then
    echo "querying ExternalIP from kubernetes cluster metadata failed, make sure your service account is properly configured."
    exit 1
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

# sharing gameserver port with gameserver containers via the sync file
echo "gameserver_port = $gameserver_port" >> $SHARED_FOLDER/$POD_NAME.syncfile
if [ $? -ne 0 ]; then
    echo "failed to create sync file"
    exit 1
fi
echo "created the sync file" $SHARED_FOLDER/$POD_NAME".syncfile and wrote port" $gameserver_port "in it"

# start the voipserver as a background process for the bash script to be able to catch signals from docker host
./$SELECTED_VOIPSERVER_BINARY ./voipserver.cfg \
    -voipserver.localport=$tunnel_port \
    -voipserver.diagnosticport=$diagnostic_port \
    -gameserver.port=$gameserver_port \
    -gameserver.maxservers=$MAX_GAMES \
    -voipserver.pingsite=$REGION \
    -voipserver.metricsaggregator.rate=30000 \
    -voipserver.metricsaggregator.port=$METRICS_PORT \
    -voipserver.metricsaggregator.addr=$METRICS_ENDPOINT \
    -voipserver.frontaddr=$public_ip \
    -voipserver.hostname=$public_hostname \
    -voipserver.deployinfo=$POD_NAME \
    -voipserver.deployinfo2=$KUBERNETES_NAMESPACE \
    -voipserver.environment=$ENVIRONMENT \
    -voipserver.maxgamesize=$MAX_GAME_SIZE \
    -voipserver.certfilename=$NUCLEUS_CERT \
    -voipserver.keyfilename=$NUCLEUS_KEY \
    -voipserver.autoscale.bufferconst=$AUTOSCALE_BUFFERCONST \
    $LOGGER_CONFIG \
    $EXTRA_CONFIG &

# we use wait to suspend execution until the child process exits.
# wait is a function that always interrupts when a signal is caught
wait $!

# save voipserver exit value
exit_code=$?
echo "The voipserver process exited with code" $exit_code

# use voipserver exit value as the exit value for the script
exit $exit_code
