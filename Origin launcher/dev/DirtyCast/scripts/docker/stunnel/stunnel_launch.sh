#!/bin/bash
# Bash script in charge of performing the final config steps and then firing the
# stunnel process

# cleanup function:delete ready file
cleanup() {
    echo "Initiating resource cleanup ..."

    # delete the ready file if it exists
    rm $SHARED_FOLDER/$POD_NAME.readyfile
    if [ $? -eq 0 ]; then
        echo "Ready file deleted"
    else
        echo "Failed to delete ready file"
    fi
}

# regular signal handler
sig_handler() {
    echo "Executing signal handler for signal" $1
    local exit_code=0

    # from here, ignore the EXIT pseudo-signal because we are explicitly calling the handler
    # (with bash, the EXIT pseudo-signal is signaled as a result of exit being called in the regular signal handler)
    trap '' EXIT

    # if stunnel is running in the background, kill it
    if [ -z $! ]; then
        echo "No need kill the stunnel process because it has not yet been started"
        exit_code=0
    else
        # on SIGTERM, do not proceed until the voipserver has completed draining
        # (so that the voipserver still gets the DELETE req from the CCS during draining)
        if [ $1 == "SIGTERM" ]; then
            while [ -f $SHARED_FOLDER/$POD_NAME.syncfile ]
            do
                echo "waiting for voipserver to delete the sync file" $SHARED_FOLDER/$POD_NAME.syncfile "..."
                sleep 5
            done
        fi

        # kill stunnel using its job identifier
        kill $!

        if [ $? -eq 0 ]; then
            echo "Successfully initiated the termination of the stunnel process running as background job" ${!}

            # we use wait to suspend execution until the child process exits.
            # wait is a function that always interrupts when a signal is caught
            wait $!

            # save stunnel exit code
            exit_code=$?
        else 
            echo "Failed to initiate the termination of the stunnel running as background job" ${!}
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

    # tell infrastructure to kill the pod
    kubectl delete pod $POD_NAME --wait=false
    if [ $? -eq 0 ]; then
        echo "stunnel container successfully initiated destruction of kub pod" $POD_NAME
    else 
        echo "stunnel container failed to initiate destruction of kub pod" $POD_NAME
    fi
}

# register the exit handler with the bash-specific EXIT (end-of-script) pseudo-signal
# the exit handler assumes the exit_code is defined
exit_code=0
trap exit_handler EXIT

echo "pid is $$"

printenv

: "${POD_NAME?Cannot run without POD_NAME env var being defined.}"
: "${NODE_NAME?Cannot run without NODE_NAME env var being defined.}"
: "${SHARED_FOLDER?Cannot run without SHARED_FOLDER env var being defined.}"
: "${SERVER_CERT?Cannot run without SERVER_CERT env var being defined.}"
: "${SERVER_KEY?Cannot run without SERVER_KEY env var being defined.}"
: "${CA_CERT?Cannot run without CA_CERT env var being defined.}"

if [ -z "$LOG_VERBOSITY" ]; then
    log_verbosity=5
else
    log_verbosity=$LOG_VERBOSITY
fi

# update config file with debug level to be used
sed -i 's#debug = x#debug = '$log_verbosity'#' stunnel.conf

# update config file with server cert/key to be used
sed -i 's#cert = xxx#cert = '$SERVER_CERT'#' stunnel.conf
sed -i 's#key = xxx#key = '$SERVER_KEY'#' stunnel.conf

# update config file with ca cert to be used
sed -i 's#CAfile = xxx#CAfile = '$CA_CERT'#' stunnel.conf

# stunnel container / voipserver container synchronization
# stunnel waits for the voipserver to create a file including the binding port
echo "synchronizing with voipserver container"
while [ ! -f $SHARED_FOLDER/$POD_NAME.syncfile ]
do
  echo "waiting for voipserver to create the sync file" $SHARED_FOLDER/$POD_NAME.syncfile "..."
  sleep 5
done

# read the restapi_port 
# expected line format in the file:  "restapi_port = XXXXX"
restapi_port=`cat $SHARED_FOLDER/$POD_NAME.syncfile | awk '{print $3}'`
echo "found port" $restapi_port "in sync file"

# get the internal IP from the k8s node metadata
local_ip=$(kubectl get nodes $NODE_NAME -o=jsonpath='{.status.addresses[?(@.type=="InternalIP")].address}')
if [ $? -ne 0 ]; then
    echo "querying InternalIP from kubernetes cluster metadata failed, make sure your service account is properly configured. now falling back to using LOCAL_IP env var instead."
    if [ -z "$LOCAL_IP" ]; then
        echo "LOCAL_IP env var not defined, exiting."
        exit 1
    fi
    local_ip=$LOCAL_IP
fi

echo "updating stunnel.conf with the port the vs is bound to:" $restapi_port
sed -i 's/accept = x.x.x.x:XXXXX/accept = '$local_ip':'$restapi_port'/' stunnel.conf
sed -i 's/connect = 127.0.0.1:XXXXX/connect = 127.0.0.1:'$restapi_port'/' stunnel.conf

# voipserver waits for stunnel to create ready file as an indication that syncrhonization is complete
echo "ready!" > $SHARED_FOLDER/$POD_NAME.readyfile
if [ $? -ne 0 ]; then
    echo "failed to create ready file to be consumed by voipserver container"
    exit 1
fi
echo "created the ready file" $SHARED_FOLDER/$POD_NAME".readyfile"

# start stunnel as a background process for the bash script to be able to catch signals from docker host
stunnel ./stunnel.conf &

# we use wait to suspend execution until the child process exits.
# wait is a function that always interrupts when a signal is caught
wait $!

# save stunnel exit value
exit_code=$?
echo "The stunnel process exited with code" $exit_code

# use stunnel exit value as the exit value for the script
exit $exit_code

