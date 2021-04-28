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

    if [ -z "$RESTAPI_PORT" ]; then
        curl -X DELETE localhost:11021/ports/$POD_NAME/RestapiPort
        if [ $? -eq 0 ]; then
            echo "Successfully cleaned up restapi port" $restapi_port
        else 
            echo "Failed to clean up restapi port" $restapi_port
        fi
    fi

    # delete the sync file if it exists
    if [ $SECURE_RESTAPI == "TRUE" ]; then
        rm $SHARED_FOLDER/$POD_NAME.syncfile
        if [ $? -eq 0 ]; then
            echo "Sync file deleted"
        else
            echo "Failed to delete sync file"
        fi
    fi

    /home/gs-tools/backupToS3.py -app=cc-hosting-server -podname="$POD_NAME" -containerid=voipserver \
        -coredir=/tmp -logdir=/home/gs-tools/voipserver/VS_Linux_CCS_0000
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
        echo "voipmetrics.events.voipservercrash:1|g|#mode:concierge,env:$ENVIRONMENT,site:$CC_HOSTING_REGION,pool:$CC_HOSTING_POOL,deployinfo:$POD_NAME,deployinfo2:$KUBERNETES_NAMESPACE,port:$diagnostic_port" > /dev/udp/$METRICS_ENDPOINT/$METRICS_PORT
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
: "${KUBERNETES_NAMESPACE?Cannot run without KUBERNETES_NAMESPACE env var being defined.}"
: "${POD_NAME?Cannot run without POD_NAME env var being defined.}"
: "${NODE_NAME?Cannot run without NODE_NAME env var being defined.}"
: "${ENVIRONMENT?Cannot run without ENVIRONMENT env var being defined.}"
: "${CC_HOSTING_REGION?Cannot run without CC_HOSTING_REGION env var being defined.}"
: "${CC_HOSTING_POOL?Cannot run without CC_HOSTING_POOL env var being defined.}"
: "${MAX_CLIENTS?Cannot run without MAX_CLIENTS env var being defined.}"
: "${SECURE_RESTAPI?Cannot run without SECURE_RESTAPI env var being defined.}"
: "${NUCLEUS_CERT?Cannot run without NUCLEUS_CERT env var being defined.}"
: "${NUCLEUS_KEY?Cannot run without NUCLEUS_KEY env var being defined.}"

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

# Find out the "restapi" port that this container should bind to
if [ -z "$RESTAPI_PORT" ]; then
    restapi_port=`curl -s localhost:11021/ports/"$POD_NAME"/RestapiPort?protocol=tcp | grep -Po '(?<=PortNumber":)[0-9]+'`
    if [ -z "$restapi_port" ]; then
        echo "Failed to get restapi port"
        exit 1
    fi
else
    restapi_port=$RESTAPI_PORT
fi
echo "restapi port:" $restapi_port

if [ -z "$NUCLEUS_ENDPOINT" ]; then
    nucleus_endpoint=https://accounts2s.int.ea.com
else
    nucleus_endpoint=$NUCLEUS_ENDPOINT
fi

if [ -z "$CCS_ENDPOINT" ]; then
    ccs_endpoint=https://ccs.integration.gameservices.ea.com
else
    ccs_endpoint=$CCS_ENDPOINT
fi

if [ -z "$LOG_VERBOSITY" ]; then
    log_verbosity=1
else
    log_verbosity=$LOG_VERBOSITY
fi

if [ -z "$METRICS_PORT" ]; then
    metrics_port=8125
else
    metrics_port=$METRICS_PORT
fi

if [ -z "$METRICS_ENDPOINT" ]; then
    metrics_endpoint=127.0.0.1
else
    metrics_endpoint=$METRICS_ENDPOINT
fi

if [ -z "$MONITOR_WRN_PCTSERVERLOAD" ]; then
    monitor_wrn_pctserverload=75
else
    monitor_wrn_pctserverload=$MONITOR_WRN_PCTSERVERLOAD
fi

if [ -z "$MONITOR_ERR_PCTSERVERLOAD" ]; then
    monitor_err_pctserverload=85
else
    monitor_err_pctserverload=$MONITOR_ERR_PCTSERVERLOAD
fi

if [ -z "$AUTOSCALE_BUFFERCONST" ]; then
    autoscale_bufferconst=2
else
    autoscale_bufferconst=$AUTOSCALE_BUFFERCONST
fi

if [ -z "$LOGGER_PRUNE_TIME" ]; then
    LOGGER_PRUNE_TIME=30
fi

if [ -z "$LOGGER_DUAL" ]; then
    LOGGER_DUAL=1
fi

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

# if RESTAPI is secured, then perform stunnel container / voipserver container synchronization
if [ $SECURE_RESTAPI == "TRUE" ]; then
    echo "synchronizing with stunnel container"

    # create the sync file that the stunnel container is waiting on (it expects to find the restapi port in there)
    if [ -z "$SHARED_FOLDER" ]; then
        echo "If SECURE_RESTAPI is TRUE, then cannot run without SHARED_FOLDER env var being defined."
        exit 1
    fi
    echo "restapi_port = $restapi_port" > $SHARED_FOLDER/$POD_NAME.syncfile
    if [ $? -ne 0 ]; then
        echo "failed to create sync file to be consumed by stunnel container"
        exit 1
    fi
    echo "created the sync file" $SHARED_FOLDER/$POD_NAME".syncfile and wrote port" $restapi_port "in it"

    # voipserver waits for stunnel container to confirm the stunnel process is beings started before starting the voipserver
    # this step is necessary to avoid the possibility of the voipserver process registering with the CCS before stunnel is up
    while [ ! -f $SHARED_FOLDER/$POD_NAME.readyfile ]
    do
        echo "waiting for stunnel to create the ready file" $SHARED_FOLDER/$POD_NAME.readyfile "..."
        sleep 5
    done

    cc_use_stunnel=1
else
    cc_use_stunnel=0
fi

# start the voipserver as a background process for the bash script to be able to catch signals from docker host
./$SELECTED_VOIPSERVER_BINARY ./voipserver.cfg \
    -voipserver.localport="$tunnel_port" \
    -voipserver.diagnosticport="$diagnostic_port" \
    -gameserver.port="$restapi_port" \
    -voipserver.offloadInboundSSL="$cc_use_stunnel" \
    -voipserver.pingsite="$CC_HOSTING_REGION" \
    -voipserver.pool="$CC_HOSTING_POOL" \
    -voipserver.nucleusaddr=$nucleus_endpoint \
    -voipserver.ccsaddr="$ccs_endpoint" \
    -voipserver.debuglevel="$log_verbosity" \
    -voipserver.maxtunnels="$MAX_CLIENTS" \
    -voipserver.metricsaggregator.rate=30000 \
    -voipserver.metricsaggregator.port="$metrics_port" \
    -voipserver.metricsaggregator.addr="$metrics_endpoint" \
    -voipserver.frontaddr="$public_ip" \
    -voipserver.hostname="$public_hostname" \
    -voipserver.deployinfo="$POD_NAME" \
    -voipserver.deployinfo2="$KUBERNETES_NAMESPACE" \
    -voipserver.environment=$ENVIRONMENT \
    -voipserver.autoscale.bufferconst="$autoscale_bufferconst" \
    -voipserver.certfilename=$NUCLEUS_CERT \
    -voipserver.keyfilename=$NUCLEUS_KEY \
    -monitor.pctserverload="$monitor_wrn_pctserverload,$monitor_err_pctserverload" \
    -logger.dual=$LOGGER_DUAL \
    -logger.cfgfile=voipserver_logger.cfg \
    -logger.logfile="VS_Linux_CCS_0000/voipserver0000.log" \
    -logger.prunetime=$LOGGER_PRUNE_TIME \
    $EXTRA_CONFIG &

# we use wait to suspend execution until the child process exits.
# wait is a function that always interrupts when a signal is caught
wait $!

# save voipserver exit value
exit_code=$?
echo "The voipserver process exited with code" $exit_code

# use voipserver exit value as the exit value for the script
exit $exit_code
