#!/bin/bash

#chmod 744 <thisfile>

# kill entire process group when exiting
trap "set +b; echo 'Finished.'; kill -- $$" EXIT

# default wipe behaviour (performed before an ilt run is started). Can be overridden in ilt.cfg.
# The options are:
# "default"      Wipe all logs and cores (client and server), for the service name configured in ilt.cfg
# "all"          Wipe all deployed data, for all service names, on all client and server boxes.
#                Cannot be used for ilt re-runs (if configured for a re-run, the wipe behaviour reverts to "default")
# "checkonly"    Don't wipe anything, but check for cores on all client and server boxes before starting the run.
#                If cores are found, the run is aborted.
# "none"         Don't wipe anything and don't check for cores before starting the run.
iltWipe="default"

# default behavior to stop an ILT run if a core file is found. Can be overridden in ilt.cfg.
iltStopOnCoreFiles=1

# default behavior is to perform zdt restarts during an ILT run. Can be overridden in ilt.cfg.
iltRestart=1

# default restart type when iltRestart is enabled.  If this setting is left empty, then a "zdt_restart" will be performed.  This is
# functionality provided by the ./fab script, and it performs a full cluster restart by restarting multiple instances in parallel.
# If this value is changed to contain a comma-separated string of instance types (e.g. "coreSlave,grSlave,coreNSMaster"), then
# ./fab will perform a serial rolling restart of just to types of instances.
iltRestartInstanceTypes=""

# default wait/delay (amount of time in minutes) before starting the cluster restart. Can be overridden in ilt.cfg.
iltRestartDelayMin=30

# default setting of whether to use SIGABRT instead of SIGKILL in server deploy script when force terminating a process. Can be overridden in ilt.cfg.
iltForceAbort=False

# default wait/delay (amount of time in minutes) between stopclient and stopserver. Can be overridden in ilt.cfg.
iltTargetPsuTimeoutDrainMin=5

# relative path of script relative to launch location
srcDir=$(dirname ${BASH_SOURCE[0]})

# absolute blaze source code directory
blazeDir=`readlink -f $srcDir/../..`

# directory of the profile we are executing, usually tools/ilt/profiles/<profilename>
profileDir="."

# config gen
genServerCfg=0
genClientCfg=0

# cleanup stages
stopServer=0
stopClient=0
checkCoreAtEnd=0

# default graphite config
iltGraphite="eadp.gos.graphite.ea.com"
iltGraphiteTorch="eadp.gos.graphite-web.bio-iad.ea.com"
iltGraphiteExport=1
iltOIExport=0
iltGraphitePrefix="eadp.gos." # used in <prefix>blaze.<env>, <prefix>redis.<env>, <prefix>torch.<env>, etc.

# default sliver namespece settings. Can be overridden in ilt.cfg
# alternatively, comment out (any or all) to let this script attempt to calculate suitable value(s)
iltGameManagerSliverCount=1000
iltGameManagerPartitionCount=0 # let the server auto-partition this namespace
iltUserSessionsSliverCount=1000

# default core slave endpoint configs ()
slavePortBindType=0 # 0 (internal interface), 1 (external interface)
slavePortAddrType=0 # 0 (INTERNAL_IPPORT), 1 (EXTERNAL_IPPORT), 2 (XBOX_SERVER_ADDRESS)

# absolute time when stress clients have been started
clientStartTimeSec=$(date +%s)

# default name of our server cluster
clusterName="core"

# default bootfile override
# if this changes, then we'll likely need to add support to use configs in places other than etc.ilt.sports
deployOverrideBootFile="../etc.ilt.sports/sports.boot"

# ssh options
sshOpts="-o BatchMode=yes -o StrictHostKeyChecking=no -o ServerAliveInterval=15"

# whether to use Attic to transfer deploy configs and tools/blazeserver tarballs
useAttic=0

# load helper functions
source $srcDir/../fetchstatusvariable/common
source $srcDir/bin/util

# Files generated when network error simulations are set up.
# networkErrorSimulationHostsFile contains a list of the hosts on which iptables commands will be run.
# networkErrorSimulationStateFile contains the number of network error simulations left to run,
# the state of the current simulation, and the time of the next state transition.
# The network error simulation states are
# -1: error state
#  0: no simulation is running (but cleanup is still required to return the ilt machines to their pre-simulation state)
#  1: a simulation is running
networkErrorSimulationHostsFile="$srcDir/bin/network_err_sim_hosts.cfg"
networkErrorSimulationStateFile="$srcDir/bin/network_err_sim_state.cfg"

# Mock external service params. See buildmockserviceparams()
deployMockServiceHosts=()
deployMockServiceHostFolder=mocksrv
deployMockServicePort=8085
deployMockServiceStartCmds=()
deployMockServiceStartCmdBeforeOptions=()
deployMockServiceConfigs=()
deployMockServiceConfigPaths=()
deployMockServiceTopStatsCmd=""

# ILT get metrics defaults
iltReportMmMetricsProtocol=httpxml # protocol to use (typically httpxml or rest) for the call to get matchmaking metrics. Will automatically hit the appropriate Blaze endpoint. The default if unspecified is httpxml.

checkcores()
{
    setupssh
    local host=$1
    local subDir=$2
    # find files in subdir under deployUser's home dir
    local coreList
    makeSudoStr $deployUser
    coreList=$(ssh $sshOpts $host "$sudoStr find \`echo ~$deployUser\` -follow -path '*$subDir*' -name '*core.*' -not -name '*cfg' -type f")
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] checkcores Failed to communicate with host. Ignore and continue."
        # One of the few cases (if there are any other cases) where we don't return a "quit script early" result.
        # Checking for cores is done regularly in a typical ILT run, and in case the communication failure is
        # temporary, we'll ignore it here and rely on other cases of communication failure to quit the run.
        return 0
    fi
    if [ "$coreList" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] Cores found @ ~$deployUser/$subDir"
        for item in ${coreList[@]}
        do
            echo "`date +%Y/%m/%d-%T` [ERR][$host]     $item"
        done
        return 1
    fi
}

checkspacekb()
{
    setupssh
    local host=$1
    local subDir=$2
    local minSpace=$3
    local freeSpace
    freeSpace=$(ssh $sshOpts $host "if [ -d \`echo ~$deployUser\`/$subDir ]; then df -P \`echo ~$deployUser\`/$subDir ; else df -P \`echo ~$deployUser\` ; fi | grep / | awk '{print \$4}'") 
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] checkspacekb Failed to communicate with host."
        return 1
    fi
    if [ "$freeSpace" -lt "$minSpace" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] Insufficient disk space (KB) $freeSpace < $minSpace @ ~$deployUser/$subDir."
        return 1
    fi
}

checkfilehandles()
{
    setupssh
    local host=$1
    local minFh=$2
    local fhCount
    fhCount=$(ssh $sshOpts $host "cat /proc/sys/fs/file-nr | awk '{print \$3}'")
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] checkfilehandles Failed to communicate with host."
        return 1
    fi
    if [ "$fhCount" -lt "$minFh" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] Insufficient max file handles $fhCount < $minFh."
        return 1
    fi
    fhCount=$(ssh $sshOpts $host "ulimit -n")
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] Failed to communicate with host."
        return 1
    fi
    if [ "$fhCount" -lt "$minFh" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] Insufficient max user file handles $fhCount < $minFh."
        return 1
    fi
}

checkprocs()
{
    setupssh
    local host=$1
    local minProc=$2
    local procCount
    procCount=$(ssh $sshOpts $host "ulimit -u")
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] checkprocs Failed to communicate with host."
        return 1
    fi
    if [ "$procCount" -lt "$minProc" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] Insufficient process limit $procCount < $minProc."
        return 1
    fi
}

checkiptables()
{
    setupssh
    local host=$1
    ssh $sshOpts $host "sudo /sbin/iptables --version > /dev/null"

    # If the user is unable to run iptables on the host, skip the check and log a warning.
    if [ $? -ne 0 ]; then
        echo "`date +%Y/%m/%d-%T` [WARN][$host] Unable to check for iptables filters."
        return 0
    fi

    # Verify that the host has no iptables chains left over from a previous ILT run.
    ssh $sshOpts $host "sudo /sbin/iptables -nL ilt_input || sudo /sbin/iptables -nL ilt_output"
    if [ $? -eq 0 ]; then
        echo "`date +%Y/%m/%d-%T` [ERR][$host] Found iptables filters from a previous ILT run."
        return 1
    fi
    return 0
}

checkenvready()
{
    setupssh
    #checkspacekb marmot.online.ea.com /opt 20000000 # 20,000,000KB ~ 20GB
    #checkfilehandles marmot.online.ea.com 300000
    for host in ${serverHosts[@]}
    do
        checkfilehandles $host $iltHostReqFreeFileHandles
        if [ $? -ne 0 ];
        then
            return 1
        fi
        checkprocs $host $iltHostReqProcLimit
        if [ $? -ne 0 ];
        then
            return 1
        fi 
        checkspacekb $host "" $iltHostReqFreeDiskSpaceKB
        if [ $? -ne 0 ];
        then
            return 1
        fi
    done

    for host in ${clientHosts[@]}
    do
        checkfilehandles $host $iltHostReqFreeFileHandles
        if [ $? -ne 0 ];
        then
            return 1
        fi
        checkprocs $host $iltHostReqProcLimit
        if [ $? -ne 0 ];
        then
            return 1
        fi 
        checkspacekb $host "stress-client-$deploySvcName" $iltHostReqFreeDiskSpaceKB
        if [ $? -ne 0 ];
        then
            return 1
        fi
    done
}

checkenvcores()
{
    setupssh
    local coresFound=0

    #checkcores marmot.online.ea.com ~
    for host in ${serverHosts[@]}
    do
        # check main deploy dir (cores dumped here before monitor zips/moves them)
        checkcores $host "$deploySvcName/$clusterName"
        if [ $? -ne 0 ];
        then
            coresFound=1
        fi
        # check gzipped cores dir
        checkcores $host "cores/$deploySvcName"
        if [ $? -ne 0 ];
        then
            coresFound=1
        fi

    done

    for host in ${clientHosts[@]}
    do
        checkcores $host "stress-client-$deploySvcName"
        if [ $? -ne 0 ];
        then
            coresFound=1
        fi
    done

    return $coresFound
}

checkenviptables()
{
    setupssh
    for host in ${serverHosts[@]}
    do
        checkiptables $host
        if [ $? -ne 0 ];
        then
            return 1
        fi
    done

    for host in ${clientHosts[@]}
    do
        checkiptables $host
        if [ $? -ne 0 ];
        then
            return 1
        fi
    done
}

checkinservicestate()
{
    local redirector=$(getredirector $deployRdir)
    echo "`date +%Y/%m/%d-%T` [INFO] Using redirector ($redirector) to fetch server list ($deploySvcName $deployRdir $deployPlat)"
    local oldIFS=$IFS
    IFS=$'\n'
    local servers
    servers=$(getservers $redirector $deploySvcName $deployRdir $deployPlat "-" "fire2" 1 0 "tcp")
    local cnt=0
    for server in ${servers[@]}
    do
        local inService=$(echo $server | awk '{ print $3 }')
        if [ $inService -eq 1 ];
        then
            let cnt=cnt+1
        fi
    done
    IFS=$oldIFS

    # get number of servers configured in ilt.cfg
    local inSvcCount=$(getnumservers)
    if [[ $cnt -lt $inSvcCount ]];
    then
        echo "`date +%Y/%m/%d-%T` [INFO] Blaze cluster incomplete, in-service instances: $cnt/$inSvcCount."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] Blaze cluster complete, in-service instances: $cnt/$inSvcCount."
}

checkpsustate()
{
    local timeoutSec=$1
    local timeoutText=""
    if [ ! -z "$timeoutSec" ] && [ $timeoutSec -gt 0 ];
    then
        timeoutText="(timeout in $(date -u -d @${timeoutSec} +%T))"
    fi

    local psu
    psu=$($srcDir/../fetchstatusvariable/fsv "$deploySvcName" "$deployRdir" "$deployPlat" configMaster --noheader --nobuildinfo --component usersessions GaugeUserSessionsOverall | awk '{ print $2 }')
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to fetch status var for $deploySvcName.$deployPlat @ $deployRdir $timeoutText"
        return 1
    fi
    if [[ "$psu" -lt "$iltTargetPsu" ]];
    then
        echo "`date +%Y/%m/%d-%T` [INFO] Insufficient PSU $psu < $iltTargetPsu $timeoutText"
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] Sufficient PSU $psu >= $iltTargetPsu"
}

checkpsudrain()
{
    local timeoutSec=$1
    local timeoutText=""
    if [ ! -z "$timeoutSec" ] && [ $timeoutSec -gt 0 ];
    then
        timeoutText="(timeout in $(date -u -d @${timeoutSec} +%T))"
    fi

    local psu
    psu=$($srcDir/../fetchstatusvariable/fsv "$deploySvcName" "$deployRdir" "$deployPlat" configMaster --noheader --nobuildinfo --component usersessions GaugeUserSessionsOverall | awk '{ print $2 }')
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to fetch status var for $deploySvcName.$deployPlat @ $deployRdir $timeoutText"
        return 1
    fi
    if [[ $psu -gt 0 ]];
    then
        echo "`date +%Y/%m/%d-%T` [INFO] Draining PSU $psu > 0 $timeoutText"
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] Drained PSU"
}

checkiltstatus()
{
    setupssh
    # check for any cores
    if [ $iltStopOnCoreFiles -ne 0 ];
    then
        checkenvcores
        if [ $? -ne 0 ];
        then
            return 1
        fi
    else
        checkCoreAtEnd=1
    fi

    genreport
    local result=$?

    if [ $result -ne 0 ];
    then
        if [ $iltIgnoreReportResult -eq 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [WARN] ILT report did not match provided profile: $iltProfile @ PSU ${targetPsuK}K, halting test."
            return $result
        else
            echo "`date +%Y/%m/%d-%T` [WARN] ILT report did not match provided profile: $iltProfile @ PSU ${targetPsuK}K, continuing test."
        fi
    else
        echo "`date +%Y/%m/%d-%T` [INFO] ILT report matched provided profile: $iltProfile @ PSU ${targetPsuK}K."    
    fi

    if [ $iltMaxNetworkErrorSimulations -ne 0 ]; then
        _getnetworkerrorsimulationfile "state"
        if [ $? -ne 0 ]; then
            return 1
        fi

        if [ $numNetworkErrorSimulations -ne 0 ] && [ $(date +%s) -ge $nextNetworkErrorSimulationTransition ]; then
            if [ $networkErrorSimulationState -eq 1 ]; then
                stopnetworkerrorsimulation
                if [ $networkErrorSimulationState -lt 0 ]; then
                    return 1
                fi
                nextNetworkErrorSimulationTransition=$(date -d "+$iltNetworkErrorSimulationDelayMin minutes" +%s)
                numNetworkErrorSimulations=$(($numNetworkErrorSimulations-1))
            elif [ $networkErrorSimulationState -eq 0 ]; then
                startnetworkerrorsimulation
                if [ $networkErrorSimulationState -lt 0 ]; then
                    return 1
                fi
                if [ $? -eq 0 ]; then
                    nextNetworkErrorSimulationTransition=$(date -d "+$iltNetworkErrorSimulationDurationMin minutes" +%s)
                fi
            fi
            _setnetworkerrorsimulationstate
        fi
    fi

    # generate metrics fetch as needed
    getmmmetrics
}

genreport()
{
    # capture current memory and cpu snapshot
    getmemcpustatus $iltReportDir/memcpu2_txt
    if [ $? -ne 0 ];
    then
        if [ $iltIgnoreReportResult -eq 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] Could not fetch memcpustatus, halting test."
            return 1
        else
            echo "`date +%Y/%m/%d-%T` [WARN] Could not fetch memcpustats, continuing test."
        fi
    fi
    # we use _ for memcpu files because we don't want them moved to prev/ using *.*
    if [ ! -f $iltReportDir/memcpu1_txt -o ! -f $iltReportDir/memcpu2_txt ];
    then
        if [ $iltIgnoreReportResult -eq 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] Required CPU and Memory status file(s) $iltReportDir/[memcpu1_txt,memcpu2_txt] not found, halting test."
            return 1
        else
            echo "`date +%Y/%m/%d-%T` [WARN] Required CPU and Memory status file(s) $iltReportDir/[memcpu1_txt,memcpu2_txt] not found, continuing test."
        fi
    fi
    local targetPsuK=$(($iltTargetPsu/1000)) # convert psu to thousands
    if [[ $targetPsuK -lt 1 ]];
    then
        # clamp PSU to 1K
        echo "`date +%Y/%m/%d-%T` [WARN] Target PSU < 1K, clamping."
        let targetPsuK=1
    fi
    local uptime=$(($(date +%s) - $clientStartTimeSec))
    local uptimeMin=$(($uptime/60))
    if [[ $uptimeMin -lt 30 ]];
    then
        # clamp uptime to 30 minutes
        echo "`date +%Y/%m/%d-%T` [INFO] ILT uptime < 30 min, clamping to 30 min for Graphite."
        let uptimeMin=30
    fi
    mkdir -p ${iltReportDir}/prev
    # always clean up prev report files, we'll be moving current files there
    rm -rf ${iltReportDir}/prev/*
    # move last set of report files into prev report dir
    if [ "$(find ${iltReportDir} -maxdepth 1 -type f -name '*.*')" ];
    then
        mv ${iltReportDir}/*.* ${iltReportDir}/prev
        echo "`date +%Y/%m/%d-%T` [INFO] Previous ILT report moved @ ${iltReportDir}/prev."
    fi
    local reportPath
    local result
    if [ "$iltOIExport" -ne 0 ];
    then
        reportPath=$($srcDir/automation/oireport.sh "$deploySvcName" "$deployRdir" "$deployPlat" $targetPsuK "$srcDir/automation/$iltProfile" $uptimeMin $iltReportDir --graphite $iltGraphiteTorch --graphite-prefix "$iltGraphitePrefix" --memcpu $iltReportDir/memcpu1_txt $iltReportDir/memcpu2_txt)
        result=$?
    else
        reportPath=$($srcDir/automation/generatereport.sh "$deploySvcName" "$deployRdir" "$deployPlat" $targetPsuK "$srcDir/automation/$iltProfile" $uptimeMin $iltReportDir --graphite $iltGraphite --graphite-prefix "$iltGraphitePrefix" --memcpu $iltReportDir/memcpu1_txt $iltReportDir/memcpu2_txt)
        result=$?
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] ILT report written @ $reportPath."

    # copy configs used in this run to the report
    cp -f -p $profileDir/ilt.cfg $iltReportDir
    cp -f -p $profileDir/envtemplate.boot $iltReportDir

    return $result
}

genserverconfig()
{
    if [ $genServerCfg -ne 0 ];
    then
        # config already generated, skip
        return 0
    fi
    
    genServerCfg=1

    # generate server config files
    local fileName="$blazeDir/bin/server_${deployEnv}_${deployPlat}.cfg"
    if [ -n "$datacenterProxy" ] && [ $datacenterProxy == $(hostname) ];
    then
        fileName="$HOME/server_${deployEnv}_${deployPlat}.cfg"
    fi
    local overWrite=""
    if [ -f "$fileName" ];
    then
        overWrite=" (overwrote existing)"
        rm -f "$fileName"
    fi
    genservercfg > $fileName
    echo "$fileName generated$overWrite."

    fileName="$configDir/etc/env/${deployEnv}.boot"
    overWrite=""
    if [ -f "$fileName" ];
    then
        overWrite=" (overwrote existing)"
        rm -f "$fileName"
    fi
    genserverboot > $fileName
    echo "$fileName generated$overWrite."

    fileName="$srcDir/bin/package.cfg"
    overWrite=""
    if [ -f "$fileName" ];
    then
        overWrite=" (overwrote existing)"
        rm -f "$fileName"
    fi
    genserverpkg > $fileName
    echo "$fileName generated$overWrite."
}

genserverpkg()
{
    echo "# non-templated settings added by ILT control script"
    echo "[repo]"
    echo "branch = $deployRepo"
    echo ""

    echo "# templated settings..."
    cat $srcDir/bin/package_common.cfg
}

genserverboot()
{
    set -f # disable globbing to avoid expanding things like bin/*
    echo "// non-templated settings added by ILT control script"
    if [[ "$deployMockXoneUrl" != "" ]] || [[ "$deployMockPs4Url" != "" ]]; then
        if [[ "$deployMockXoneUrl" != "" ]] && ([[ "$deployPlat" == "xone" ]] || [[ "$deployPlat" == "common" ]]); then
            echo "#define MOCK_EXTERNAL_SERVICE_URL_XONE \"$deployMockXoneUrl\""
        fi
        if [[ "$deployMockPs4Url" != "" ]] && ([[ "$deployPlat" == "ps4" ]] || [[ "$deployPlat" == "common" ]]); then
            echo "#define MOCK_EXTERNAL_SERVICE_URL_PS4 \"$deployMockPs4Url\""
        fi
    elif [[ "$deployMock1stPartyUrl" != "" ]]; then
        if [[ "$deployPlat" == "xone" ]]; then
            echo "#define MOCK_EXTERNAL_SERVICE_URL_XONE \"$deployMock1stPartyUrl\""
        elif [[ "$deployPlat" == "ps4" ]]; then
            echo "#define MOCK_EXTERNAL_SERVICE_URL_PS4 \"$deployMock1stPartyUrl\""
        fi
    fi
    if [ -n "$datacenterProxy" ];
    then
        echo "#define ILT_POOL \"$iltPoolSanitized\""
        echo "#define ILT_DOMAIN \"$vmDomain\""
    fi
    if [ $iltGraphiteExport -ne 0 ];
    then
        echo "#define GRAPHITE_PREFIX \"$iltGraphitePrefix"blaze."$deployRdir\""
    fi
    echo "#define GAMEMANAGER_SLIVERCOUNT $iltGameManagerSliverCount"
    echo "#define GAMEMANAGER_PARTITIONCOUNT $iltGameManagerPartitionCount"
    echo "#define USERSESSIONS_SLIVERCOUNT $iltUserSessionsSliverCount"
    echo ""
    echo "// templated settings..."
    while read line
    do
        echo $(eval "echo \"$line\"")
    done < $profileDir/envtemplate.boot
    set +f # reenable globbing
}

genservercfg()
{
    echo "[DEPLOY]"
    echo "env              = $deployRdir"
    echo "platform         = $deployPlat"
    echo "user             = $deployUser"
    echo "bootfile         = env/$deployEnv.boot"
    echo "overrideBootFile = $deployOverrideBootFile"
    echo "serviceName      = $deploySvcName"
    echo "serviceNamePrefix= $deploySvcNamePrefix"
    echo "oldBuildCount    = 1"
    echo ""
    echo "[DEFAULT_CMDLINE]"
    echo "blazeserver:"
    echo " %(var:buildPath)s/bin/blazeserver -l %(DEPLOY:label)s --name %(var:instanceName)s"
    echo " --type %(var:instanceType)s --id %(var:instanceId)s -p %(DEPLOY:runDir)s/blaze_%(var:instanceName)s.pid"
    echo " -DPLATFORM=%(DEPLOY:platform)s -DENV=%(DEPLOY:env)s -DBLAZE_SERVICE_NAME_PREFIX=%(DEPLOY:serviceNamePrefix)s"
    echo " --logdir %(DEPLOY:logDir)s --logname blaze_%(var:instanceName)s %(DEPLOY:bootfile)s --bootFileOverride %(DEPLOY:overrideBootFile)s"
    echo ""
    echo "[LOGSTASH]"
    echo "pin_url = https://pin-river-lt-server-internal.data.ea.com/pinEvents"
    echo ""
    echo "[SETTINGS]"
    echo "usePas           = False"
    echo "startPort        = $portBase"
    echo "portsPerInstance = $portsPerServer"
    echo "forceAbort       = $iltForceAbort"
    echo ""
    echo "[MAIL]"
    echo "recipients       = $deployMail"
    echo ""

    # collect all the hosts and instance types
    local allConfigs="$configMaster:configMaster:1"
    local allTypes="configMaster"
    for master in ${masters[@]}
    do
        allConfigs+=($master)
        # convert row to array of elements
        local masterArr=($(echo "$master" | tr ":" " "))
        local masterType=${masterArr[1]}
        # only add unique elements
        case "${allTypes[@]}" in *"$masterType"*) ;; *)
            allTypes+=($masterType) ;;
        esac
    done
    for slave in ${slaves[@]}
    do
        allConfigs+=($slave)
        # convert row to array of elements
        local slaveArr=($(echo "$slave" | tr ":" " "))
        local slaveType=${slaveArr[1]}
        # only add unique elements
        case "${allTypes[@]}" in *"$slaveType"*) ;; *)
            allTypes+=($slaveType) ;;
        esac
    done
    
    local redisClusters=""
    local allRedisConfigs=""
    for redis in ${redis[@]}
    do
        allRedisConfigs+=($redis)
        # convert row to array of elements
        local redisArr=($(echo "$redis" | tr ":" " "))
        local redisClusterName=${redisArr[1]}
        # only add unique elements
        case "${redisClusters[@]}" in *"$redisClusterName"*) ;; *)
            redisClusters+=($redisClusterName) ;;
        esac
    done
    
    # sort my lists
    allConfigs=($(for each in ${allConfigs[@]}; do echo $each; done | sort))
    allRedisConfigs=($(for each in ${allRedisConfigs[@]}; do echo $each; done | sort))
    allTypes=($(for each in ${allTypes[@]}; do echo $each; done | sort -u))
    
    unset allHosts
    for config in ${allConfigs[@]}
    do
        local configArr=($(echo "$config" | tr ":" " "))
        local configHost=${configArr[0]}
        case "${allHosts[@]}" in *"$configHost"*) ;; *)
            allHosts+=($configHost) ;;
        esac
    done     

    echo "[CLUSTERS]"
    echo "core = configMaster coreMaster coreNSMaster coreSlave grSlave mmSlave searchSlave pkrMaster pkrSlave"
    echo "custom = exampleAuxMaster exampleAuxSlave"
    echo "gdpr = gdprAuxSlave"
    echo ""
    
    echo "[REDIS]"
    echo "clusters = ${redisClusters[@]}"
    
    local prevHost=""
    local startid=1
    local curHostCount=0
    # setup each host (configs should be grouped by host)
    for config in ${allConfigs[@]}
    do
        # convert row to array of elements
        local configArr=($(echo "$config" | tr ":" " "))
        local configHost=${configArr[0]}
        local configType=${configArr[1]}
        local configCount=${configArr[2]}
        
        if [[ "$configHost" != "$prevHost" ]];
        then
            let curHostCount+=1
            echo ""
            echo "[$configHost]"
            echo "startId: $startid"
            echo "runset:"
            prevHost=($configHost)
        fi

        echo "  $configType = $configCount"
        let startid+=configCount
        let index+=1
    done
    
    local prevRedisHost=""
    local redisIndex=0
    local redisPort=$portRedisBase
    # setup each redis host (configs should be grouped by host)
    for config in ${allRedisConfigs[@]}
    do
        # convert row to array of elements
        local configArr=($(echo "$config" | tr ":" " "))
        local redisHost=${configArr[0]}
        local clusterName=${configArr[1]}
        local redisCount=${configArr[2]}
        
        if [[ "$redisHost" != "$prevRedisHost" ]];
        then
            echo ""
            echo "[$redisHost]"
            echo "redis_nodes:"
            prevRedisHost=($redisHost)
            # reset the redis base port for every host
            redisPort=$portRedisBase
        fi
        
        local index=0
        while [ $index -lt $redisCount ]
        do
            local redisNode="$clusterName-$redisIndex"
            echo "  $redisNode = $redisPort"
            let redisPort+=1
            # redis node names are unique in the same cluster across all hostnames
            let redisIndex+=1
            let index+=1
        done
        
        local slaveIndex=0
        while [ $slaveIndex -lt $redisCount ];
        do
            local redisNode="$clusterName-$slaveIndex:slave"
            echo "  $redisNode = $redisPort"
            let redisPort+=1
            let slaveIndex+=1
        done
    done
    
    echo ""
}

genclientconfig()
{
    if [ $genClientCfg -ne 0 ];
    then
        # config already generated, skip
        return 0
    fi
    
    genClientCfg=1
    
    # generate client config files
    local fileName="$srcDir/bin/clientdeploy.cfg"
    local overWrite=""
    if [ -f "$fileName" ];
    then
        overWrite=" (overwrote existing)"
        rm -f "$fileName"
    fi
    genclientcfg > $fileName
    echo "$fileName generated$overWrite."
}

_genserverports()
{
    local slavePortChannel=$1

    # grab server list from redirector
    redirector=$(getredirector $deployRdir)
    echo "`date +%Y/%m/%d-%T` [INFO] Using redirector ($redirector) to fetch server ports ($deploySvcName $deployRdir $deployPlat $slavePortChannel)" >/dev/tty

    local oldIFS=$IFS
    IFS=$'\n'
    local fetchedServers
    fetchedServers=$(getservers $redirector $deploySvcName $deployRdir $deployPlat "coreSlave" "fire2" $slavePortBindType $slavePortAddrType $slavePortChannel)
    for server in ${fetchedServers[@]}
    do
        local slaveHost=$(echo $server | awk '{ print $1 }')
        local port=$(echo $server | awk '{ print $2 }')
        echo "${slavePortChannel}serverports+=(\"${port}\") # $slaveHost"
    done
    IFS=$oldIFS
}

genclientcfg()
{
    local runName=$deploySvcName    
    local redirector=$(getredirector_client $deployRdir)
    echo "runname=$runName"
    echo "clientName=stress-client-$runName"
    echo "configDir=$configDir"
    if [ -z "$datacenterProxy" ];
    then
        echo "buildDir=$blazeDir/build"
        echo "stressBuildDir=$blazeDir/build"
    else
        echo "buildDir=~/iltscripts/build"
        echo "stressBuildDir=~/stressclient"
    fi
    if [ -n "$clientSecureOverride" ];
    then
        echo "stressSecureOverride=\"-DSTRESS_SECURE=$clientSecureOverride\"" # SSL certs are not installed on EnT ILT hosts
    fi
    echo "rampup=$iltRampUpMin"
    echo "clientuser=$deployUser"
    echo "clientpath=~$deployUser/stress-client-$runName"
    if [ $iltClientUseRedirector -ne 0 ];
    then
        echo "clientUsesRedirector=1"
    else
        echo "clientUsesRedirector=0"
    fi
    echo "serviceNamePrefix=$deploySvcNamePrefix"
    echo "serviceName=$deploySvcName"
    echo "redirectorAddress=$redirector"    

    for client in ${clients[@]}
    do
        local clientArr=($(echo "$client" | tr ":" " "))
        local clientHost=${clientArr[0]}
        local clientCount=${clientArr[1]}
        echo "clienthost+=(\"$clientHost\")"
    done

    local clientRunStart=1
    for client in ${clients[@]}
    do
        local clientArr=($(echo "$client" | tr ":" " "))
        local clientHost=${clientArr[0]}
        local clientCount=${clientArr[1]}
        local clientRunEnd=$((clientRunStart+clientCount-1))
        echo "clientrun+=(\"$clientRunStart $clientRunEnd\") # $clientHost/$clientCount"
        let clientRunStart=$((clientRunEnd+1))
    done

    for slave in ${slaves[@]}
    do
        # convert row to array of elements
        local slaveArr=($(echo "$slave" | tr ":" " "))
        local slaveHost=${slaveArr[0]}
        local slaveType=${slaveArr[1]}
        local slaveCount=${slaveArr[2]}
        if [ "$slaveType" = "coreSlave" ];
        then
            echo "serverhost+=(\"$slaveHost\") # $slaveType/$slaveCount"
        fi
    done

    _genserverports "tcp"
    _genserverports "ssl"
    
    for slave in ${slaves[@]}
    do
        # convert row to array of elements
        local slaveArr=($(echo "$slave" | tr ":" " "))
        local slaveHost=${slaveArr[0]}
        local slaveType=${slaveArr[1]}
        local slaveCount=${slaveArr[2]}
        if [[ "$slaveType" != "coreSlave" ]];
        then
            echo "auxserverhost+=(\"$slaveHost\") # $slaveType/$slaveCount"
        fi
    done

    for master in ${masters[@]}
    do
        # convert row to array of elements
        local masterArr=($(echo "$master" | tr ":" " "))
        local masterHost=${masterArr[0]}
        local masterType=${masterArr[1]}
        local masterCount=${masterArr[2]}
        echo "masterhost+=(\"$masterHost\") # $masterType/$masterCount"
    done

    echo "allHosts=\$(echo \"\${masterhost[@]} \${serverhost[@]} \${auxserverhost[@]} \${clienthost[@]}\" | tr ' ' '\\n' | sort -u) # combine hosts"

    local numConns=0
    for stressinstance in ${stressinstances[@]}
    do
        # convert row to array of elements
        local stressArr=($(echo "$stressinstance" | tr ":" " "))
        local stressConns=${stressArr[1]}
        if [ $stressConns -gt 0 ];
        then
            let numConns+=stressConns
            echo "stressinstances+=(\"$stressinstance\")"
        fi
    done
    echo "totalstressconnections=$numConns # for all stress instances in this set"

    # set mock testing for clients. Side: buildmockserviceparams() validates mock svc url and platform correctly specified
    if [[ "$deployMockXoneUrl" != "" ]] || [[ "$deployMockPs4Url" != "" ]]; then
        if [[ "$deployPlat" == "xone" ]] || [[ "$deployPlat" == "ps4" ]] || [[ "$deployPlat" == "common" ]]; then
            echo "useMockConsoleServices=true"
        fi
    elif [[ "$deployMock1stPartyUrl" != "" ]]; then
        if [[ "$deployPlat" == "xone" ]]; then
            echo "useMockConsoleServices=true"
        elif [[ "$deployPlat" == "ps4" ]]; then
            echo "useMockConsoleServices=true"
        fi
    fi
    
    echo "deployPlat=$deployPlat"
    echo "useStressLogin=$useStressLogin"

    local archiveDir="$blazeDir/bin/archive"
    if [ -n "$datacenterProxy" ] && [ $datacenterProxy == $(hostname) ];
    then
        archiveDir="`readlink -f ~/archive`"
    fi
    echo "archiveDir=$archiveDir"
    local img="`tar xfO $archiveDir/blaze-*.tar.gz taginfo.txt`"
    echo "dockerImage=$img"
}

makeserver()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    if [ ! -f "/opt/gs/blazepkg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires /opt/gs/blazepkg."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] make server."
    local pkgcfg=$(realpath $srcDir/bin/package.cfg)
    local serverfiles=$(realpath $blazeDir/bin/server_${deployEnv}_${deployPlat}.cfg)
    pushd $blazeDir
    rm -rf bin/archive
    echo "`date +%Y/%m/%d-%T` [INFO] Executing: sudo /opt/gs/blazepkg -c $pkgcfg pack --stage -b debug --rebuild-images -t $iltTag"
    sudo /opt/gs/blazepkg -c $pkgcfg pack --stage -b debug --rebuild-images -t $iltTag
    local ret=$?
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to make server package, rc: $ret."
        popd
        return 1
    fi
    local imagesOnly="--images-only"
    if [ $useAttic -ne 0 ]; then
        imagesOnly=""
    else
        # Copy deploy configs to archives folder, since they're not being pushed to Attic
        mkdir -p bin/archive/deploycfgs
        cp bin/base.cfg bin/fabenv.ini bin/fabilt.ini bin/server_${deployEnv}_${deployPlat}.cfg bin/archive/deploycfgs/
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] Executing: sudo /opt/gs/blazepkg -c $pkgcfg post --tags $iltTag --serverfiles $serverfiles --skip-older-archives $imagesOnly"
    sudo /opt/gs/blazepkg -c $pkgcfg post --tags $iltTag --serverfiles $serverfiles --skip-older-archives $imagesOnly
    ret=$?
    popd
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to post server package, rc: $ret."
        return 1
    fi
}

_genconfigdir()
{
    if [ ! -d ~/blaze ];
    then
        local blazecfgs=($(ls -a ~/archive/ | grep blaze))
        local blazetar=${blazecfgs[0]}
        if [ -z "$blazetar" ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] No blaze configs found (no blaze tarball at ~/archive and no ~/blaze directory)"
            return 1
        fi
        mkdir ~/blaze
        tar -xf ~/archive/$blazetar -C ~/blaze
    fi
}

updateblazeconfigs()
{
    pushd ~/blaze >/dev/null
    tar -zcf ../archive/blaze.tar.gz *
    local ret=$?
    popd >/dev/null
    return $ret
}

deployserver()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    if [ ! -f "/opt/gs/fab" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires /opt/gs/fab."
        return 1
    fi
    # add server to clean
    echo "`date +%Y/%m/%d-%T` [INFO] deploy server."
    stopServer=1
    pushd ~

    if [ -z "$deployServerOptions" ]; then
        deployServerOptions="dbdestructive=yes"
    fi

    setupssh
    # deploy command should also clean out (remove) any old deploys
    if [ -z "$deployProxy" ]; then
        eval "/opt/gs/fab --config=fabilt.ini --set override_prompts=yes cfg:server_${deployEnv}_${deployPlat}.cfg deploy:$deployServerOptions"
    else
        echo "`date +%Y/%m/%d-%T` [ERROR] deployProxy option is no longer supported."
        popd
        return 1
    fi
    local ret=$?
    popd
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to deploy server package, rc: $ret."
        return 1
    fi
}

startserver()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    if [ ! -f "/opt/gs/fab" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires /opt/gs/fab."
        return 1
    fi
    # add server to clean
    echo "`date +%Y/%m/%d-%T` [INFO] start server."
    setupssh
    stopServer=1
    pushd ~
    eval /opt/gs/fab --config=fabilt.ini --set override_prompts=yes cfg:server_${deployEnv}_${deployPlat}.cfg servers.start:dbdestructive=yes
    local ret=$?
    popd
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to start servers, rc: $ret."
        return 1
    fi
}

restartdelay()
{
    local sleepTimeSec=$((60*$iltRestartDelayMin))

    if [ -z "$1" ]
    then
        echo "`date +%Y/%m/%d-%T` [INFO] delaying $(date -u -d @${sleepTimeSec} +%T)"
    else
        let sleepTimeSec=$1
    fi

    sleep $sleepTimeSec
    return 0
}

restartservertype()
{
    local serverType=$1
    if [ -z "${serverType}" ] && [ -n "${iltRestartInstanceTypes}" ]; then
        serverType="t:${iltRestartInstanceTypes}"
    fi

    if [ ! -f "/opt/gs/fab" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires /opt/gs/fab."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] restart server."
    pushd ~
    if [ -z "${serverType}" ]; then
        eval /opt/gs/fab --config=fabilt.ini --set override_prompts=yes cfg:server_${deployEnv}_${deployPlat}.cfg servers.zdt-restart:dbdestructive=no,restart_custom=yes
    else
        eval /opt/gs/fab --config=fabilt.ini --set override_prompts=yes cfg:server_${deployEnv}_${deployPlat}.cfg ${serverType} servers.restart:rolling=yes
    fi
    local ret=$?
    popd
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to restart servers ($servertype), rc: $ret."
        return 1
    fi

    return 0
}

restartserver()
{
    local stages=()

    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig

    setupssh
    stages+=(restartservertype) # restart all instances

    #stages+=('restartservertype "t:configMaster"')
    #stages+=('restartservertype "t:configNSMaster"')
    #stages+=('restartservertype "t:coreMaster"')
    #stages+=('restartservertype "t:coreSlave"')
    #stages+=('restartservertype "t:grSlave"')
    #stages+=('restartservertype "t:mmSlave"')
    #stages+=('restartservertype "t:searchSlave"')
    #stages+=('restartservertype "t:exampleAuxMaster,exampleAuxSlave"')

    stages+=(checkpsustate)

    local result
    for stage in ${stages[@]}
    do
        echo "`date +%Y/%m/%d-%T` [INFO] stage --> $stage"
        eval "$stage"
        result=$?
        if [ $result -ne 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] stage <-- $stage"
            exit 1
        fi
        echo "`date +%Y/%m/%d-%T` [INFO] stage <-- $stage"
    done

    exit 0
}

reconfigure()
{
    # check if replacement files exist in profile directory
    count=`ls -1 $profileDir/*.tar.gz 2>/dev/null | wc -l`
    if [ $count != 0 ]; then 
        setupssh
        local clientPath="stress-client-$deploySvcName"
        makeSudoStr $deployUser
    
        for file in $profileDir/*.tar.gz; do
            fullname="${file##*/}" #without path
            filename="${fullname%%.*}" #without extension
            
            for host in ${serverHosts[@]}
            do
                local serverpath
                serverpath=$(ssh $sshOpts $host "$sudoStr find \`echo ~$deployUser\` -wholename '*$deploySvcName/$clusterName'")
                if [ $? -ne 0 ]; then
                    echo "`date +%Y/%m/%d-%T` [ERR][$host] reconfigure Failed to communicate with host. Ignore and continue."
                    # One of the few cases (if there are any other cases) where we don't return a "quit script early" result.
                    return 0
                fi
                serverpath=$(ssh $sshOpts $host "$sudoStr readlink -f $serverpath")
                echo "Deploying to host: $host..."
                ssh -o BatchMode=yes -o StrictHostKeyChecking=no $host "$sudoStr chmod a+rwx $serverpath";
                scp $fullname $host:$serverpath;
                ssh -o BatchMode=yes -o StrictHostKeyChecking=no $host "$sudoStr chmod a+rw $serverpath/; cd $serverpath/; $sudoStr tar -xzf $fullname"
                echo "Done."
            done

            for host in ${clientHosts[@]}
            do
                echo "Deploying to host: $host..."
                local clientpath=$(ssh $sshOpts $host "$sudoStr find \`echo ~$deployUser\` -name 'stress-client-$deploySvcName'")
                ssh -o BatchMode=yes -o StrictHostKeyChecking=no $host "$sudoStr chmod a+rwx $clientpath";
                scp $fullname $host:$clientpath;
                ssh -o BatchMode=yes -o StrictHostKeyChecking=no $host "$sudoStr chmod a+rw $clientpath/; cd $clientpath/; $sudoStr tar -xvzf $fullname"
                echo "Done."
            done      
        done

        # update config package with new files
        updateblazeconfigs
        
    fi 
}

restartserverafterdelay()
{
    restartdelay
    reconfigure
    restartserver
}

_backgroundrollingrestart()
{
    ${srcDir}/iltctrl.sh restartserverafterdelay &
}

stopserver()
{
    local stopKind="$1"
    if [ -z "$stopKind" ]; then
        stopKind="stop"
    fi

    # stop associated mock external service if its running
    stopmockservice

    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    setupssh
    makeSudoStr $deployUser
    for host in ${serverHosts[@]}
    do
        echo "`date +%Y/%m/%d-%T` [INFO][$host] stopserver(checking for restart process)..."
        ssh $sshOpts $host "$sudoStr ps aux | grep 'server restart' | grep -w 'rolling' | grep -v 'bash'"
        local pidList=($(ssh $sshOpts $host "$sudoStr ps aux | grep 'server restart' | grep -w 'rolling' | grep -v 'bash' | awk '{print \$2}'"))
        if [ $? -ne 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR][$host] stopserver(checking for restart process) Failed to communicate with host. Ignore and continue."
        else
            echo "`date +%Y/%m/%d-%T` [INFO][$host] found ${#pidList[@]} restart process(es)"
            case "${#pidList[@]}" in
            0)
                ;;
            1)
                echo "`date +%Y/%m/%d-%T` [INFO][$host] killing restart process ${pidList[0]} ..."
                ssh $sshOpts $host "$sudoStr kill ${pidList[0]}"
                ;;
            *)
                echo "`date +%Y/%m/%d-%T` [WARN][$host] unable to discern restart process"
                ;;
            esac
        fi
    done

    if [ ! -f "/opt/gs/fab" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires /opt/gs/fab."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] stop server."
    stopServer=0
    pushd ~ 
    eval /opt/gs/fab --config=fabilt.ini --set override_prompts=yes cfg:server_${deployEnv}_${deployPlat}.cfg servers.${stopKind}:checkrdir=no servers.redis-stop:hard=yes
    local ret=$?
    popd
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to stop servers, rc: $ret."
        return 1
    fi
}

_stripstress()
{
    pushd $blazeDir >/dev/null
    local blazevers=$(ls $blazeDir/build/blazeserver)
    if [ -f build/blazeserver/$blazevers/unix64-clang-debug/bin/stress ];
    then
        cp -f build/blazeserver/$blazevers/unix64-clang-debug/bin/stress build/blazeserver/$blazevers/unix64-clang-debug/bin/stress_stripped
        strip build/blazeserver/$blazevers/unix64-clang-debug/bin/stress_stripped
    fi
    popd >/dev/null
}

makeclient()
{
    # the client config depends on servers connected to redirector, so disregard any previous passes of regenerating it
    genClientCfg=0

    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    if [ ! -f "$srcDir/bin/makeclientpkg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires $srcDir/bin/makeclientpkg."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] make client."
    _stripstress # Strip debug symbols to reduce the size of the stress client binary
    eval $srcDir/bin/makeclientpkg
    local ret=$?
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to make client package, rc: $ret."
        return 1
    fi
}

deployclient()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    if [ ! -f "$srcDir/bin/deployclientpkg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires $srcDir/bin/deployclientpkg."
        return 1
    fi
    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] deploy client."
    setupssh
    if [ -z "$deployProxy" ]; then
        eval $srcDir/bin/deployclientpkg
    else
        echo "`date +%Y/%m/%d-%T` [ERROR] deployProxy option is no longer supported."
        return 1
    fi
    local ret=$?
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to deploy client package, rc: $ret."
        return 1
    fi
}

startclient()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    if [ ! -f "$srcDir/bin/stressctrl" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires $srcDir/bin/stressctrl."
        return 1
    fi
    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] start client."
    stopClient=1
    setupssh
    eval $srcDir/bin/stressctrl start
    local ret=$?
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to start clients, rc: $ret."
        return 1
    fi
}

stopclient()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    if [ ! -f "$srcDir/bin/stressctrl" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires $srcDir/bin/stressctrl."
        return 1
    fi
    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] stop client."
    setupssh
    stopClient=0
    eval $srcDir/bin/stressctrl stop
    local ret=$?
    # ret=0 means we killed the process, ret=1 means process was
    # not found, the result is the same so don't warn about it.
    if [ $ret -gt 1 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to stop clients, rc: $ret."
        return 1
    fi
}

# wipe client and server logs for the configured service name
wipelogs()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig

    if [ ! -f "$srcDir/bin/stressctrl" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires $srcDir/bin/stressctrl."
        return 1
    fi
    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] cleaning client logs."
    setupssh
    eval $srcDir/bin/stressctrl wipelogs
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe client logs."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] cleaning server logs."
    eval $srcDir/bin/wipeserver wipelogs
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe server logs."
        return 1
    fi
}

# wipe client and server cores for the configured service name
wipecores()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig

    if [ ! -f "$srcDir/bin/stressctrl" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires $srcDir/bin/stressctrl."
        return 1
    fi
    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] wiping client cores."
    setupssh
    eval $srcDir/bin/stressctrl wipecores
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe client cores."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] wiping server cores."
    eval $srcDir/bin/wipeserver wipecores
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe server cores."
        return 1
    fi
}

# Clear docker image cache on server hosts
wipedocker()
{
    genserverconfig
    genclientconfig

    echo "`date +%Y/%m/%d-%T` [INFO] Wiping docker image cache."
    local archiveDir="$blazeDir/bin/archive"
    if [ -n "$datacenterProxy" ] && [ $datacenterProxy == $(hostname) ];
    then
        archiveDir="`readlink -f ~/archive`"
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] archiveDir=$archiveDir"
    local img="`tar xfO $archiveDir/blaze-*.tar.gz taginfo.txt`"
    echo "`date +%Y/%m/%d-%T` [INFO] img=$img"
 
    echo "`date +%Y/%m/%d-%T` [INFO] Wiping client docker image cache."
    setupssh
    eval $srcDir/bin/stressctrl wipedocker $img
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe client docker image cache."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] Wiping server docker image cache."
    eval $srcDir/bin/wipeserver wipedocker $img
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe server docker image cache."
        return 1
    fi
}

# wipe all client and server deployed data for the configured service name 
wipeall()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig

    if [ ! -f "$srcDir/bin/stressctrl" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $0 requires $srcDir/bin/stressctrl."
        return 1
    fi
    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] cleaning client dirs."
    eval $srcDir/bin/stressctrl wipeall
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe client dirs."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] cleaning server dirs."
    setupssh
    eval $srcDir/bin/wipeserver wipeall
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe server dirs."
        return 1
    fi
}

# wipe all client and server deployed data for all service names
wipeboxes()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig

    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi

    setupssh
    echo "`date +%Y/%m/%d-%T` [INFO] cleaning all boxes."
    eval $srcDir/bin/wipeall
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to wipe all boxes."
        return 1
    fi
}


stopmonitors()
{
    # always regen configs, to make sure we are operating on latest ones
    genserverconfig
    genclientconfig
    
    echo "`date +%Y/%m/%d-%T` [INFO] Stop monitor processes."
    
    if [ ! -f "$srcDir/bin/clientdeploy.cfg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] $srcDir/bin/clientdeploy.cfg not found."
        return 1
    fi
    setupssh
    eval $srcDir/bin/killmon
    if [ ! $? -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to kill monitor processes."
        return 1
    fi
}

repopulatedb()
{
    if [ ${#clients[@]} -lt 1 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] At least one client host must be configured to populate db."
        return 1
    fi
    setupssh
    local runName=$deploySvcName
    local clientUser=$deployUser
    local clientPath="stress-client-$runName"
    local client=${clients[0]}
    local clientArr=($(echo "$client" | tr ":" " "))
    local clientHost=${clientArr[0]}
    makeSudoStr $clientUser
    ssh $sshOpts $clientHost "cd ~$clientUser/$clientPath/etc; $sudoStr ../bin/stress -c ../etc.ilt.sports/stress/stress-integrated.cfg -DTEST_CONFIG=../etc.ilt.sports/stress/data-population/data-create.cfg"
    if [ $? -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to populate db."
        return 1
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] Repopulated db."
}

stopall()
{
    stopclient
    checkpsudrainstage
    stopserver
}

killall()
{
    stopclient
    stopserver "kill"
}

_cleanup()
{
    if [ $1 -ne 0 ] && [ $stopServer -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [WARN] attempt to generate final report on ILT run FAILURE..."
        genreport
    fi

    echo "`date +%Y/%m/%d-%T` [INFO] cleaning up..."

    if [ -f $networkErrorSimulationStateFile ] || [ -f $networkErrorSimulationHostsFile ]; then
        teardownnetworkerrorsimulations
    fi

    if [ $stopClient -ne 0 ];
    then
        stopclient
    fi

    if [ $stopServer -ne 0 ];
    then
        checkpsudrainstage
        stopserver
    fi

    if [ $checkCoreAtEnd -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [INFO] did not check for cores during ILT run. checking now for informational purposes..."
        checkenvcores
    fi

    echo "`date +%Y/%m/%d-%T` [INFO] cleanup done."

    if [ $1 -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [INFO] ILT run SUCCEEDED."
    else
        echo "`date +%Y/%m/%d-%T` [WARN] ILT run FAILED in stage ($2)."
    fi

    return 0
}

invokeuntil()
{
    local func=$1
    local minPollPeriod=$2
    local duration=$3
    local curTime=$(date +%s)
    local startTime=$curTime
    local finishTime=$(($curTime + $duration))
    local ret=0
    while [ $curTime -lt $finishTime ];
    do
        local nextTime=$(($curTime + $minPollPeriod))
        if [ $nextTime -gt $finishTime ];
        then
            nextTime=$finishTime
        fi
        echo "`date +%Y/%m/%d-%T` [INFO] invoke: $func, remaining: $(date -u -d @$(($finishTime - $curTime)) +%T)"
        eval "$func"
        ret=$?
        if [ $ret -ne 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] invoked: $func, result: $ret"
            break
        fi
        echo "`date +%Y/%m/%d-%T` [INFO] invoked: $func, result: OK"
        curTime=$(date +%s)
        if [ $curTime -lt $nextTime ];
        then
            local sleepExtra=$(($nextTime - $curTime))
            echo "`date +%Y/%m/%d-%T` [INFO] sleep extra $(date -u -d @${sleepExtra} +%T)"
            sleep $sleepExtra
            curTime=$(date +%s)
        fi
    done
    return $ret
}

getnumservers()
{
    local serverCount=1 # always have config master
    for master in ${masters[@]}
    do
        # convert row to array of elements
        local masterArr=($(echo "$master" | tr ":" " "))
        local masterCount=${masterArr[2]}
        let serverCount+=masterCount
        # echo "severCount: $serverCount"
    done
    for slave in ${slaves[@]}
    do
        # convert row to array of elements
        local slaveArr=($(echo "$slave" | tr ":" " "))
        local slaveCount=${slaveArr[2]}
        let serverCount+=slaveCount
        # echo "serverCount: $serverCount"
    done
    echo "$serverCount"
}

getmemcpustatus()
{
    local memCpuStatusFile=$1
    eval $srcDir/../fetchstatusvariable/fsv "$deploySvcName" "$deployRdir" "$deployPlat" - --noheader --nobuildinfo --component blazeinstance cpuusageforprocesspercent processresidentmemory > $memCpuStatusFile
    if [ $? -ne 0 ];
    then

        if [ $iltIgnoreReportResult -eq 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] Failed to obtain CPU and Memory status from $deploySvcName $deployRdir $deployPlat, halting test."
            return 1
        else
            echo "`date +%Y/%m/%d-%T` [WARN] Failed to obtain CPU and Memory status from $deploySvcName $deployRdir $deployPlat, continuing test."
        fi
    else
        echo "`date +%Y/%m/%d-%T` [INFO] CPU and Memory status written to file: $memCpuStatusFile."
    fi
}

checkinservicestatestage()
{
    local result
    local sleepTimeSec=10
    local startTime=$(date +%s)
    local finishTime=$(($startTime + 60*$iltInServiceTimeoutMin))
    while [ $(date +%s) -lt $finishTime ]
    do
        checkinservicestate
        result=$?
        if [ $result -eq 0 ];
        then
            break
        fi
        sleep $sleepTimeSec
    done

    if [ $result -ne 0 ];
    then
        local timeoutTime=$(($(date +%s) - $startTime))
        echo "`date +%Y/%m/%d-%T` [ERR] checkinservicestate (timed out after $(date -u -d @${timeoutTime} +%T))"
        return $result
    fi
}

startclientstage()
{
    startclient
    local result=$?
    if [ $result -ne 0 ];
    then
        return $result
    fi
    # remember when clients were started (used by health/perf criteria check)
    clientStartTimeSec=$(date +%s)
}

checkpsustatestartup()
{
    _checkpsustatestage checkpsustate $iltTargetPsuTimeoutStartupMin
}

checkpsustatepreshutdown()
{
    if [ -z "$iltTargetPsuTimeoutPreShutdownMin" ]; then
        checkpsustate
    else
        _checkpsustatestage checkpsustate $iltTargetPsuTimeoutPreShutdownMin
    fi
}

checkpsudrainstage()
{
    _checkpsustatestage checkpsudrain $iltTargetPsuTimeoutDrainMin
}

_checkpsustatestage()
{
    if [ -z "$2" ]
    then
        echo "`date +%Y/%m/%d-%T` [ERR] checkpsustatestage called without enough parameters (condition_param timeout_param)"
        return 1
    fi

    echo "`date +%Y/%m/%d-%T` [INFO] ILT target PSU timeout configured to: $2 minutes"

    local result
    local sleepTimeSec=60
    local startTime=$(date +%s)
    local finishTime=$(($startTime + 60*$2))
    while [ $(date +%s) -lt $finishTime ]
    do
        eval "$1 $(($finishTime - $(date +%s)))"
        result=$?
        if [ $result -eq 0 ];
        then
            break
        fi
        if [ $iltStopOnCoreFiles -ne 0 ];
        then
            checkenvcores
            if [ $? -ne 0 ];
            then
                return 1
            fi
        else
            checkCoreAtEnd=1
        fi
        sleep $sleepTimeSec
    done

    if [ $result -ne 0 ]
    then
        local timeoutTime=$(($(date +%s) - $startTime))
        echo "`date +%Y/%m/%d-%T` [ERR] $1 (timed out after $(date -u -d @${timeoutTime} +%T))"
        return $result
    fi
}

checkiltstatusstage()
{
    if [ $iltMaxNetworkErrorSimulations -ne 0 ]; then
        # The next network error simulation transition needs to be reset to start from this time,
        # not from the time setupnetworkerrorsimulations was called. But first check that setup
        # completed successfully
        _getnetworkerrorsimulationfile "state"
        if [ $? -ne 0 ]; then
            return 1
        fi
        nextNetworkErrorSimulationTransition=$(date -d "+$iltNetworkErrorSimulationDelayMin minutes" +%s)
        _setnetworkerrorsimulationstate
    fi

    # always try to create report directory
    mkdir -p $iltReportDir
    # always clean up report directory
    rm -rf $iltReportDir/*
    getmemcpustatus $iltReportDir/memcpu1_txt
    if [ $? -ne 0 ];
    then
        return 1
    fi
    local runDurationSec=$((60*$iltTargetPsuRunDurationMin))
    invokeuntil checkiltstatus 60 $runDurationSec
    local result=$?
    if [ $result -ne 0 ];
    then
        return $result
    fi

    if [ $iltMaxNetworkErrorSimulations -ne 0 ]; then
        if [ $networkErrorSimulationState -lt 0 ]; then
            # details of the error should already have been logged elsewhere
            return 1
        fi

        completedNetworkErrorSimulations=$(($iltMaxNetworkErrorSimulations-$numNetworkErrorSimulations))
        if [ $networkErrorSimulationState -eq 1 ]; then
            echo "`date +%Y/%m/%d-%T` [WARN] ILT run ended while a network error simulation was running. The simulation will be stopped."
            stopnetworkerrorsimulation
            completedNetworkErrorSimulations=$(($completedNetworkErrorSimulations-1))
        fi

        if [ $completedNetworkErrorSimulations -eq 0 ]; then
            echo "`date +%Y/%m/%d-%T` [ERR] Network error simulations were configured, but none were completed."
            return 1
        fi
    fi
}

getmmmetrics()
{
    if [[ "$iltReportMmMetricsProtocol" != "" ]]; then
        echo "`date +%Y/%m/%d-%T` [INFO] iltReportMmMetricsProtocol set to: $iltReportMmMetricsProtocol"
        local redirector=$(getredirector $deployRdir)

        local metricsServersAndPorts=$(getservers $redirector $deploySvcName $deployRdir $deployPlat "mmSlave" $iltReportMmMetricsProtocol 1)

        local iltMetricsDir="${iltReportDir}"/getmmmetrics
        mkdir -p ${iltMetricsDir}        
        local filepathprefix="${iltMetricsDir}"/at`date +%s`_
        for metricServer in "${metricsServersAndPorts[@]}"
        do
            local metricServerAddr=$(echo $metricServer | awk '{ print $2 }')
            local metricsfilename=${filepathprefix}`echo ${metricServerAddr} | tr \: '_'`_txt
            echo "`date +%Y/%m/%d-%T` [INFO] http://${metricServerAddr}/matchmaker/getMatchmakingMetrics"
            curl ${curlOpts[*]} "http://$metricServerAddr/matchmaker/getMatchmakingMetrics" > "${metricsfilename}" &
            echo "`date +%Y/%m/%d-%T` [INFO] mm metrics saved to file \"${metricsfilename}\""
        done
    fi
}

geniltpkgcfg()
{
    local blazevers=$(ls $blazeDir/build/blazeserver)

    set -f # disable globbing to avoid expanding things like bin/*
    echo "# non-templated settings added by ILT control script"
    echo "[repo]"
    echo "branch = $deployRepo"
    echo ""
    echo "[pack ilt]"
    echo "includes ="
    echo "    build/blazeserver/${blazevers}/unix64-clang-debug/bin/stress_stripped"
    echo "    build/blazeserver/${blazevers}/unix64-clang-debug/container_lib/*"
    echo "    tools/mockexternalservice/*"
    echo "    tools/fetchstatusvariable/*"
    echo "    tools/ilt/bin/*"
    echo "    tools/ilt/automation/*"
    echo "    tools/ilt/iltctrl.sh"
    echo "    tools/ilt/profiles/*.cfg"
    echo "    tools/ilt/profiles/$(basename $PWD)/*"
    echo "excludes ="
    echo "    tools/mockexternalservice/win64"
    echo "    tools/mockexternalservice/*.sln"
    echo "    tools/mockexternalservice/*/*.njsproj"
    set +f # reenable globbing
}

prepdcproxy()
{
    if [ ! -f "/opt/gs/blazepkg" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] /opt/gs/blazepkg required to make ILT package (to send to deploy proxy host)."
        return 1
    fi

    local fileName=$(realpath $srcDir/bin/package_ilt.cfg)
    local overWrite=""
    if [ -f "$fileName" ];
    then
        overWrite=" (overwrote existing)"
        rm -f "$fileName"
    fi
    geniltpkgcfg > $fileName
    echo "$fileName generated$overWrite."

    echo "`date +%Y/%m/%d-%T` [INFO] make ILT package."
    # strip stress binary, to avoid consuming too much disk space on the proxy box
    _stripstress

    pushd $blazeDir
    eval sudo /opt/gs/blazepkg -c $fileName pack --no-stage -t $iltTag
    local ret=$?
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to make ILT package, rc: $ret."
        popd
        return 1
    fi
    if [ $useAttic -ne 0 ]; then
        echo "`date +%Y/%m/%d-%T` [INFO] Executing: sudo /opt/gs/blazepkg -c $fileName post --tags $iltTag --skip-older-archives"
        sudo /opt/gs/blazepkg -c $fileName post --tags $iltTag --skip-older-archives
        ret=$?
    fi
    popd
    if [ $ret -ne 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] Failed to post ILT package, rc: $ret."
        return 1
    fi
}

runiltprep()
{
    if [ -z "$datacenterProxy" ];
    then
        echo "`date +%Y/%m/%d-%T` [ERR] runiltprep command is only valid when datacenterProxy is set"
        exit 1
    fi

    local stages=(makeserver prepdcproxy)
    local result
    for stage in ${stages[@]}
    do
        echo "`date +%Y/%m/%d-%T` [INFO] stage --> $stage"
        eval "$stage"
        result=$?
        if [ $result -ne 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] stage <-- $stage"
            exit 1
        fi
        echo "`date +%Y/%m/%d-%T` [INFO] stage <-- $stage"
    done

    local copyScriptsCmd="rm -rf iltscripts; mkdir iltscripts; cd iltscripts; mv ../archive/\$(ls ../archive | grep \"ilt\" | tail -1) .; tar -xvf ilt-*"
    echo ""
    echo "######################################### MANUAL STEPS REQUIRED #########################################"
    echo ""
    echo "(1) SSH to $datacenterProxy. Clear your home directory of any previous ILT artifacts (*.cfg files, fab*.ini files, and archive, deploy, and iltscripts folders)."
    echo ""
    if [ $useAttic -eq 0 ]; then
        echo "(2) scp your blazeserver/bin/archive folder to your home directory on $datacenterProxy. Then, from your home directory on $datacenterProxy, execute:"
        echo ""
        echo "    mv archive/deploycfgs/* .; rm -rf archive/deploycfgs; $copyScriptsCmd"
    else
        echo "(2) From your home directory, execute:"
        echo ""
        echo "    /opt/gs/fab test:$deploySvcName pull:branch=$deployRepo,tag=$iltTag,latestonly=yes; $copyScriptsCmd"
    fi
    echo ""
    echo "(3) From $datacenterProxy, start your ILT run from ~/iltscripts/tools/ilt/profiles/$(basename $PWD) ."
    echo ""
    echo "##########################################################################################################"

    exit 0
}

runiltwithzdt()
{
    iltRestart=1
    _runilt 0
}

runiltnozdt()
{
    iltRestart=0
    _runilt 0
}

runilt()
{
    _runilt 0
}

rerunilt()
{
    _runilt 1
}

_runilt()
{
    setupssh
    local stages=()

if [ -n "$datacenterProxy" ] && [ $datacenterProxy != $(hostname) ]; then
    echo "######################################### WARNING #########################################"
    echo ""
    echo "ILT runs may only be executed from your configured datacenterProxy, $datacenterProxy"
    echo ""
    echo "###########################################################################################"
    return 1
fi

    case "$iltWipe" in
    default)
        stages+=(wipelogs)
        stages+=(wipecores)
        ;;
    all)
        if [ $1 -eq 0 ];
        then
            stages+=(wipeboxes)
        else
            echo "`date +%Y/%m/%d-%T` [WARN] iltWipe option 'all' is invalid for ILT re-runs. Using option 'default' instead."
            stages+=(wipelogs)
            stages+=(wipecores)
        fi
        ;;
    checkonly)
        stages+=(checkenvcores)
        ;;
    none)
        ;;
    *)
        echo "`date +%Y/%m/%d-%T` [ERR] Invalid option for iltWipe: $iltWipe"
        return 1
    esac

    stages+=(checkenvready)
    stages+=(checkenviptables)
    stages+=(deploymockservice)

    if [ $1 -eq 0 ];
    then
        if [ -z "$datacenterProxy" ];
        then
            stages+=(makeserver)
        else
            stages+=(updateblazeconfigs)
        fi
        stages+=(wipedocker)
        stages+=(deployserver) # includes startserver
    else
        stages+=(startserver)
    fi

#    stages+=(stopmonitors) # enable to avoid having monitors restart servers
    stages+=(checkinservicestatestage)
    #stages+=(reconfigure)
    if [ $1 -eq 0 ];
    then
        stages+=(makeclient) # client config gen depends on server having registered with rdir
        stages+=(deployclient)
    fi

    if [ -z "$iltRepopulateDb" ] || [ $iltRepopulateDb -eq 0 ];
    then
        echo "`date +%Y/%m/%d-%T` [INFO] db population: OFF"
    else
        echo "`date +%Y/%m/%d-%T` [INFO] db population: ON"
        stages+=(repopulatedb) # this should be safe even with the server up, as long as clients are not running
    fi
    stages+=(setupnetworkerrorsimulations)
    stages+=(startclientstage)
    stages+=(checkpsustatestartup)

    if [ $iltRestart -ne 0 ];
    then
        stages+=(_backgroundrollingrestart) # kick off a command in the background that will execute a rolling restart
    fi

    stages+=(checkiltstatusstage)
    stages+=(checkpsustatepreshutdown)

    local result
    for stage in ${stages[@]}
    do
        echo "`date +%Y/%m/%d-%T` [INFO] stage --> $stage"
        eval "$stage"
        result=$?
        if [ $result -ne 0 ];
        then
            echo "`date +%Y/%m/%d-%T` [ERR] stage <-- $stage"
            _cleanup 1 $stage
            exit 1
        fi
        echo "`date +%Y/%m/%d-%T` [INFO] stage <-- $stage"
    done

    _cleanup 0

    exit 0
}

dotest()
{
    setupssh
    #genserverconfig
    #genclientconfig
    #repopulatedb
    #echo "test"
    checkenvcores
    if [ $? -ne 0 ];
    then
    exit 1
    fi
}

_getnetworkerrorsimulationfile()
{
    if [ "$1" = "all" ] || [ "$1" = "state" ]; then
        if [ -z "$networkErrorSimulationState" ]; then
            source $networkErrorSimulationStateFile
            if [ $? -ne 0 ]; then
                if [ -n "$2" ]; then
                    echo "`date +%Y/%m/%d-%T` [ERR] (${FUNCNAME[1]}) Could not find file $networkErrorSimulationStateFile."
                fi
                return 1
            fi
            if [ -z "$networkErrorSimulationState" ]; then
                if [ -n "$2" ]; then
                    echo "`date +%Y/%m/%d-%T` [ERR] (${FUNCNAME[1]}) Network error simulations were not set up successfully."
                fi
                return 1
            fi
        fi
    fi

    if [ "$1" = "all" ] || [ "$1" = "hosts" ]; then
        if [ -z "$iptablesAffectedHosts" ]; then
            source $networkErrorSimulationHostsFile
            if [ $? -ne 0 ]; then
                if [ -n "$2" ]; then
                    echo "`date +%Y/%m/%d-%T` [ERR] (${FUNCNAME[1]}) Could not find file $networkErrorSimulationHostsFile."
                fi
                return 1
            fi
        fi
    fi

    return 0
}

_setnetworkerrorsimulationstate()
{
    > $networkErrorSimulationStateFile
    oldIFS=$IFS
    IFS=$'\n'
    echo "networkErrorSimulationState=\"${networkErrorSimulationState}\"" >> $networkErrorSimulationStateFile
    echo "numNetworkErrorSimulations=\"${numNetworkErrorSimulations}\"" >> $networkErrorSimulationStateFile
    echo "nextNetworkErrorSimulationTransition=\"${nextNetworkErrorSimulationTransition}\"" >> $networkErrorSimulationStateFile
    IFS=$oldIFS
}

setupnetworkerrorsimulations()
{
    if [ -f $networkErrorSimulationStateFile ] || [ -f $networkErrorSimulationHostsFile ]; then
        echo "`date +%Y/%m/%d-%T` [ERR] A previous network error simulation has not been torn down."
        return 1
    fi

    unset networkErrorSimulationState

    if [ -z "$portBase" ] || [ $portBase -lt 1024 ]; then
        echo "`date +%Y/%m/%d-%T` [ERR] Configured portBase is missing or is a system port."
        _setnetworkerrorsimulationstate
        return 1
    fi

    if [ $iltMaxNetworkErrorSimulations -eq 0 ]; then
        return 0
    fi

    source "$srcDir/bin/clientdeploy.cfg"
    local serverports=()

    if [ ${#tcpserverports[@]} -gt 0 ]; then
        serverports+=("${tcpserverports[@]}")
    fi
    if [ ${#sslserverports[@]} -gt 0 ]; then
        serverports+=("${sslserverports[@]}")
    fi

    if [ ${#serverports[@]} -eq 0 ]; then
        echo "`date +%Y/%m/%d-%T` [ERR] (setupnetworkerrorsimulations) No servers listed in clientdeploy.cfg."
        _setnetworkerrorsimulationstate
        return 1
    fi

    declare -A serversAndPorts
    for hostAndPort in ${serverports[@]};
    do
        # collect list of hosts and ports to create iptables filters
        local arrHostAndPort=(${hostAndPort//:/ })
        local ports=${serversAndPorts[${arrHostAndPort[0]}]}
        if [ -z "$ports" ]; then
            serversAndPorts[${arrHostAndPort[0]}]=${arrHostAndPort[1]}
        else
            ports+=",${arrHostAndPort[1]}"
            serversAndPorts[${arrHostAndPort[0]}]=$ports
        fi
    done

    setupssh
    local oldIFS=$IFS
    > $networkErrorSimulationHostsFile
    for host in "${!serversAndPorts[@]}"; do
        IFS=$'\n'
        echo "iptablesAffectedHosts+=($host)" >> $networkErrorSimulationHostsFile
        IFS=$oldIFS

        ssh $sshOpts $host "sudo /sbin/iptables -N ilt_input"
        local result=$?
        if [ $result -eq 0 ]; then
            ssh $sshOpts $host "sudo /sbin/iptables -N ilt_output"
            result=$?
            if [ $result -ne 0 ]; then
                ssh $sshOpts $host "sudo /sbin/iptables -X ilt_input"
            fi
        fi

        if [ $result -ne 0 ]; then
            echo "`date +%Y/%m/%d-%T` [ERR] (setupnetworkerrorsimulations) Failed to create iptables chains on host $host."
            networkErrorSimulationState=-1
            _setnetworkerrorsimulationstate
            return 1
        fi

        for client in ${clientHosts[@]}; do
            ssh $sshOpts $host "sudo /sbin/iptables -A ilt_output -d $client -p tcp -m multiport --dports ${serversAndPorts[$host]} -j DROP; sudo /sbin/iptables -A ilt_input -s $client -p tcp -m multiport --dports ${serversAndPorts[$host]} -j DROP"
            if [ $? -ne 0 ]; then
                echo "`date +%Y/%m/%d-%T` [ERR] (setupnetworkerrorsimulations) Failed to add iptables rules on host $host."
                networkErrorSimulationState=-1
                _setnetworkerrorsimulationstate
                return 1
            fi
        done
    done

    networkErrorSimulationState=0
    numNetworkErrorSimulations=$iltMaxNetworkErrorSimulations
    nextNetworkErrorSimulationTransition=$(date -d "+$iltNetworkErrorSimulationDelayMin minutes" +%s)
    _setnetworkerrorsimulationstate
    return 0
}

teardownnetworkerrorsimulations()
{
    # Source the file containing network error simulation state information and check
    # whether simulation setup progressed to a stage after which teardown would be required. 
    # Add a second parameter to suppress error logging.
    _getnetworkerrorsimulationfile "state" "nologging"
    if [ $? -ne 0 ]; then
        echo "`date +%Y/%m/%d-%T` [INFO] (teardownnetworkerrorsimulations) Network error simulations were not set up. No iptables changes reverted."
        if [ -f $networkErrorSimulationStateFile ]; then
            rm -f $networkErrorSimulationStateFile
        fi
        if [ -f $networkErrorSimulationHostsFile ]; then
            rm -f $networkErrorSimulationHostsFile
        fi
        return 0
    fi

    stopnetworkerrorsimulation
    result=0
    for host in "${iptablesAffectedHosts[@]}"; do
        ssh $sshOpts $host "sudo /sbin/iptables -F ilt_input; sudo /sbin/iptables -X ilt_input; sudo /sbin/iptables -F ilt_output; sudo /sbin/iptables -X ilt_output"
        if [ $? -ne 0 ]; then
            # Note: its possible iptables failed due to reasons other than uncleaned rules. Fail if there are still lingering rules.
            checkiptables $host
            if [ $? -ne 0 ]; then
                result=1
                echo "`date +%Y/%m/%d-%T` [ERR] (teardownnetworkerrorsimulations) Failed to revert iptables changes on host $host."
            fi
        fi
    done

    if [ $result -eq 0 ]; then
        rm -f $networkErrorSimulationStateFile
        rm -f $networkErrorSimulationHostsFile
    else
        networkErrorSimulationState=-1
        _setnetworkerrorsimulationstate
        return 1
    fi

    return 0
}

startnetworkerrorsimulation()
{
    _getnetworkerrorsimulationfile "all"
    if [ $? -ne 0 ]; then
        return 1
    fi

    setupssh
    if [ $networkErrorSimulationState -ne 0 ]; then
        echo "`date +%Y/%m/%d-%T` [WARN] Attempted to start a network error simulation while a simulation was already running, or in an error state."
        return 1
    fi

    checkpsustate
    if [ $? -ne 0 ]; then
        echo "`date +%Y/%m/%d-%T` [INFO] PSU is below target; network error simulation not started."
        return 1
    fi

    networkErrorSimulationState=1
    for host in "${iptablesAffectedHosts[@]}"; do
        echo "`date +%Y/%m/%d-%T` [INFO] Beginning network error simulation on host $host"
        ssh $sshOpts $host "sudo /sbin/iptables -A OUTPUT -j ilt_output; sudo /sbin/iptables -A INPUT -j ilt_input"

        if [ $? -ne 0 ]; then
            echo "`date +%Y/%m/%d-%T` [ERR] Failed to start network error simulation on host $host."
            networkErrorSimulationState=-1
        fi
    done

    _setnetworkerrorsimulationstate
    return 0
}

stopnetworkerrorsimulation()
{
    _getnetworkerrorsimulationfile "all"
    if [ $? -ne 0 ]; then
        return 1
    fi

    setupssh
    if [ $networkErrorSimulationState -ne 0 ]; then
        networkErrorSimulationState=0
        for host in "${iptablesAffectedHosts[@]}"; do
            echo "`date +%Y/%m/%d-%T` [INFO] Ending network error simulation on host $host"
            ssh $sshOpts $host "sudo /sbin/iptables -D OUTPUT -j ilt_output; sudo /sbin/iptables -D INPUT -j ilt_input"
            if [ $? -ne 0 ]; then
                echo "`date +%Y/%m/%d-%T` [ERR] Failed to stop network error simulation on host $host."
                networkErrorSimulationState=-1
            fi
        done
    fi

    _setnetworkerrorsimulationstate

    if [ $networkErrorSimulationState -ne 0 ]; then
        return 1
    fi

    return 0
}

checkrunning()
{
    setupssh
    for host in ${serverHosts[@]}
    do
        echo "`date +%Y/%m/%d-%T` [INFO] $host"
        ssh $sshOpts $host "ps aux | egrep 'blaze|stress'"
    done

    for host in ${clientHosts[@]}
    do
        echo "`date +%Y/%m/%d-%T` [INFO] $host"
        ssh $sshOpts $host "ps aux | egrep 'blaze|stress'"
    done
}


# build mock external service params. Sets deployMockServiceHost to non-empty host string, if mock service is enabled
buildmockserviceparams()
{
    setupssh
    local deployMockServiceFiles=()
    local deployMockAddressArr=()
    deployMockServiceHosts=()
    
    # define all mock services we want to use
    # if xone/ps4 urls are specified we choose those over the legacy 1st party url config    
    if [[ "$deployMockXoneUrl" != "" ]] || [[ "$deployMockPs4Url" != "" ]]; then
        if [[ "$deployMockXoneUrl" != "" ]] && ([[ "$deployPlat" == "xone" ]] || [[ "$deployPlat" == "common" ]]); then
            deployMockServiceFiles+=("mpsdserver.js")
            deployMockServiceConfigs+=("mpsdserver_ilt.cfg")       
            deployMockAddressArr+=($(echo "$deployMockXoneUrl" | tr ":" " "))            
        fi
        if [[ "$deployMockPs4Url" != "" ]] && ([[ "$deployPlat" == "ps4" ]] || [[ "$deployPlat" == "common" ]]); then
            deployMockServiceFiles+=("psnserver.js")
            deployMockServiceConfigs+=("psnserver_ilt.cfg")
            deployMockAddressArr+=($(echo "$deployMockPs4Url" | tr ":" " "))
        fi
        
    elif [[ "$deployMock1stPartyUrl" != "" ]]; then
        if [[ "$deployPlat" == "xone" ]]; then
            deployMockServiceFiles+=("mpsdserver.js")
            deployMockServiceConfigs+=("mpsdserver_ilt.cfg")
        elif [[ "$deployPlat" == "ps4" ]]; then
            deployMockServiceFiles+=("psnserver.js")
            deployMockServiceConfigs+=("psnserver_ilt.cfg")
        else
            echo "`date +%Y/%m/%d-%T` [ERR] deployMock1stPartyUrl was specified but deployPlat was $deployPlat. ILT's Blaze Server cfgs expect the PLATFORM to be xone or ps4 in order to propertly test mock service."
            return 1
        fi
        deployMockAddressArr=($(echo "$deployMock1stPartyUrl" | tr ":" " "))
        
    fi    
    
    for i in "${!deployMockServiceFiles[@]}"; do
        deployMockServiceHosts+=(${deployMockAddressArr[$i*2]})
        # override the default port, with any specified in the mock service url below
        if [[ ${#deployMockServiceFiles[@]} > $i*2+1  ]]; then
            deployMockServicePort=${deployMockAddressArr[$i*2+1]}
        fi   
        
        deployMockServiceConfigPaths+=("$blazeDir/tools/ilt/profiles/${deployMockServiceConfigs[$i]}")

        deployMockServiceStartCmdBeforeOptions+=("./node --nouse-idle-notification --max_old_space_size=8192 ${deployMockServiceFiles[$i]} -port=${deployMockServicePort}")

        # Add extra options to cmd line.
        deployMockServiceStartCmds+=("${deployMockServiceStartCmdBeforeOptions[$i]} -config=${deployMockServiceConfigs[$i]}")
       
    done
}

# deploy and start mock external web service
deploymockservice()
{
    local deployNodeJS_tar=$blazeDir/tools/mockexternalservice/linux64/nodejs.tgz
    local deployMockService_tar=$blazeDir/tools/mockexternalservice/mocksrv.tgz
    local deployMockServiceSrcFilesDir=()
    local origDir=$PWD
    
    if [[ "$deployPlat" == "common" ]] && [ "$deployMockXoneUrl" != "" ]; then
        deployMockServiceSrcFilesDir+=($blazeDir/tools/mockexternalservice/mockexternalservicexone)
    fi
    if [[ "$deployPlat" == "common" ]] && [ "$deployMockPs4Url" != "" ]; then
        deployMockServiceSrcFilesDir+=($blazeDir/tools/mockexternalservice/mockexternalserviceps4)
    fi
    if [[ "$deployPlat" != "common" ]]; then
        deployMockServiceSrcFilesDir+=($blazeDir/tools/mockexternalservice/mockexternalservice$deployPlat)
    fi
    
    stopmockservice
    
    buildmockserviceparams  
    
    if [[ ${#deployMockServiceHosts[@]} == 0 ]]; then
        return 0;
    fi

    # deploy and launch all mock service types provided 
    for i in "${!deployMockServiceHosts[@]}"; do
    
        # zip source files and push them to host
        echo
        echo "[${deployMockServiceHosts[$i]}] Pushing mock external service files $deployMockService_tar."
        if [ ! -d $deployMockServiceSrcFilesDir ]; then
            echo "`date +%Y/%m/%d-%T` [ERR] Mock external service source files dir ${deployMockServiceSrcFilesDir[$i]} does not exist."
            return 1;
        fi
        cd $deployMockServiceSrcFilesDir/
        sudo cp ${deployMockServiceConfigPaths[$i]} .
        tar czf ${deployMockService_tar} `ls *.js` `ls ../common/*.js`  ${deployMockServiceConfigs[$i]}
        if [ $? -ne 0 ]; then
            echo "`date +%Y/%m/%d-%T` [ERR] Failed to zip mock service files."
            return 1
        fi
        cd $origDir
        cat ${deployMockService_tar} | ssh $sshOpts ${deployMockServiceHosts[$i]} "mkdir --parents ${deployMockServiceHostFolder} && tar xvzf - -C ${deployMockServiceHostFolder} && cp -rf ${deployMockServiceHostFolder}/common ${deployMockServiceHostFolder}/.."
        if [ $? -ne 0 ]; then
            echo "`date +%Y/%m/%d-%T` [ERR] Failed to copy mock external service files to target host."
            return 1
        fi

        # copy NodeJS
        echo "[${deployMockServiceHosts[$i]}] Pushing $deployNodeJS_tar to ~/${deployMockServiceHostFolder}"
        cat ${deployNodeJS_tar} | ssh $sshOpts ${deployMockServiceHosts[$i]} "mkdir --parents ${deployMockServiceHostFolder} && tar xvzf - -C ${deployMockServiceHostFolder}"
        if [ $? -ne 0 ]; then
            echo "`date +%Y/%m/%d-%T` [ERR] Failed to copy Node JS executable to target host. This is required to run the mock service."
            return 1
        fi
        
        # start service
        echo "[${deployMockServiceHosts[$i]}] Executing: ${deployMockServiceStartCmds[$i]}"
        ssh $sshOpts ${deployMockServiceHosts[$i]} "cd ${deployMockServiceHostFolder}; ${deployMockServiceStartCmds[$i]} > /dev/null 2>&1 &"
        # confirm its running
        if ssh $sshOpts -qn ${deployMockServiceHosts[$i]} "ps aux" | grep -q "${deployMockServiceStartCmds[$i]}"; then
            echo "`date +%Y/%m/%d-%T` [INFO] Deployed mock external service."
        else
            echo "`date +%Y/%m/%d-%T` [ERR] Failed to start mock external service on target host."
            return 1
        fi;

        # log cpu
        local nodePid=`ssh $sshOpts ${deployMockServiceHosts[$i]} "pgrep -f \"${deployMockServiceStartCmdBeforeOptions[$i]}\" " `
        if [[ "$nodePid" != "" ]]; then
            local topstatsCount=$((50*$iltTargetPsuRunDurationMin))
            deployMockServiceTopStatsCmd="top -b -c -n ${topstatsCount} -p ${nodePid}"
            ssh $sshOpts ${deployMockServiceHosts[$i]} "cd ${deployMockServiceHostFolder}; ${deployMockServiceTopStatsCmd} > cpuMockSrv.log &"
            if [ $? -ne 0 ]; then
                echo "[${deployMockServiceHosts[$i]}] Failed to run top stats for mock external service."
                deployMockServiceTopStatsCmd=""
            fi
        fi
        
    done
}

stopmockservice()
{
    setupssh
    buildmockserviceparams
    if [[ "${deployMockServiceHost[@]}" == 0 ]]; then
        return 0;
    fi
    
    for i in "${!deployMockServiceHosts[@]}"; do
        # first move present logs to a zip
        ssh $sshOpts ${deployMockServiceHosts[$i]} "cd ${deployMockServiceHostFolder};" '_tmpLogList=`find . -type f -print | grep \.log$`; if [[ \"$_tmpLogList\" != \"\" ]]; then tar czf logs`date +%s`.tgz $_tmpLogList; rm $_tmpLogList; find ~ -type d -empty -delete; fi;'

        echo
        echo "[${deployMockServiceHosts[$i]}] Stopping mock service instances matching command line: ${deployMockServiceStartCmdBeforeOptions[$i]}"
        ssh $sshOpts ${deployMockServiceHosts[$i]} "pkill -f \"${deployMockServiceStartCmdBeforeOptions[$i]}\" "
        if [ $? -ne 0 ]; then
            echo "[${deployMockServiceHosts[$i]}] Failed to stop any running mock external service, or no service was running."
            return 0
        fi
        if [[ "$deployMockServiceTopStatsCmd" != "" ]]; then
            ssh $sshOpts ${deployMockServiceHosts[$i]} "pkill -f \"${deployMockServiceTopStatsCmds[$i]}\" "
            if [ $? -ne 0 ]; then
                echo "[${deployMockServiceHosts[$i]}] Failed to stop any running mock external service top stats command, or no such command was running."
            fi
        fi
    done
}

setupssh()
{
    if [ -z "$SSH_AUTH_SOCK" ] ; then
        eval `ssh-agent`
        ssh-add
    fi
}


# build client and server host arrays
clientHosts=()
serverHosts=($configMaster)

buildhostarrays()
{
    local totalSets=0
    local numCoreSlaves=0
    local numSearchSlaves=0

    for client in ${clients[@]}
    do
        local clientArr=($(echo "$client" | tr ":" " "))
        local clientHost=${clientArr[0]}
        local numSets=${clientArr[1]}
        # only add unique elements
        case "${clientHosts[@]}" in *"$clientHost"*) ;; *)
            let totalSets+=numSets
            clientHosts+=($clientHost) ;;
        esac
        # echo "+=$clientHost"
    done
    
    for master in ${masters[@]}
    do
        # convert row to array of elements
        local masterArr=($(echo "$master" | tr ":" " "))
        local masterHost=${masterArr[0]}
        # only add unique elements
        case "${serverHosts[@]}" in *"$masterHost"*) ;; *)
            serverHosts+=($masterHost) ;;
        esac
        # echo "+=$masterHost"
    done

    for slave in ${slaves[@]}
    do
        # convert row to array of elements
        local slaveArr=($(echo "$slave" | tr ":" " "))
        local slaveHost=${slaveArr[0]}
        local slaveType=${slaveArr[1]}
        local slaveCount=${slaveArr[2]}

        case "$slaveType" in
        coreSlave)
            let numCoreSlaves+=slaveCount
            ;;
        searchSlave)
            let numSearchSlaves+=slaveCount
            ;;
        *)
            ;;
        esac

        # only add unique elements
        case "${serverHosts[@]}" in *"$slaveHost"*) ;; *)
            serverHosts+=($slaveHost) ;;
        esac
        # echo "+=$slaveHost"
    done

    # count the number of connections in each test set
    local numConns=0
    for stressinstance in ${stressinstances[@]}
    do
        # convert row to array of elements
        local stressArr=($(echo "$stressinstance" | tr ":" " "))
        local stressName=${stressArr[0]}
        local stressConns=${stressArr[1]}
        local stressConfig=${stressArr[2]}
        if [ $stressConns -gt 0 ];
        then
            local fileName="$configDir/etc/$stressConfig"
            if [ ! -f "$fileName" ];
            then
                echo "`date +%Y/%m/%d-%T` [ERR] stress client config for $stressName doesn't exist: $fileName"
                exit 1
            fi
            let numConns+=stressConns
        fi
    done

    local totalConns
    let totalConns=totalSets*numConns
    echo "`date +%Y/%m/%d-%T` [INFO] total client connections configured: $totalConns"

    if [ -z "$iltTargetPsu" ];
    then
        let iltTargetPsu=totalConns*96/100
        let iltTargetPsu/=1000
        let iltTargetPsu*=1000
        echo "`date +%Y/%m/%d-%T` [INFO] iltTargetPsu calculated to: $iltTargetPsu"
    else
        echo "`date +%Y/%m/%d-%T` [INFO] iltTargetPsu configured to: $iltTargetPsu"
    fi

    if [ -z "$iltGameManagerSliverCount" ];
    then
        # ceil(sqrt(numCoreSlaves))
        let iltGameManagerSliverCount=$(echo "sqrt($numCoreSlaves)" | bc -l | awk '{ printf "%d",($0%int($0))?int($0)+(($0>0)?1:0):$0 }')
        let iltGameManagerSliverCount*=20
        echo "`date +%Y/%m/%d-%T` [INFO] iltGameManagerSliverCount calculated to: $iltGameManagerSliverCount"
    else
        echo "`date +%Y/%m/%d-%T` [INFO] iltGameManagerSliverCount configured to: $iltGameManagerSliverCount"
    fi

    if [ -z "$iltGameManagerPartitionCount" ];
    then
        # ceil(sqrt(numSearchSlaves))
        let iltGameManagerPartitionCount=$(echo "sqrt($numSearchSlaves)" | bc -l | awk '{ printf "%d",($0%int($0))?int($0)+(($0>0)?1:0):$0 }')
        echo "`date +%Y/%m/%d-%T` [INFO] iltGameManagerPartitionCount calculated to: $iltGameManagerPartitionCount"
    else
        echo "`date +%Y/%m/%d-%T` [INFO] iltGameManagerPartitionCount configured to: $iltGameManagerPartitionCount"
    fi

    if [ -z "$iltUserSessionsSliverCount" ];
    then
        let iltUserSessionsSliverCount=numCoreSlaves*10
        echo "`date +%Y/%m/%d-%T` [INFO] iltUserSessionsSliverCount calculated to: $iltUserSessionsSliverCount"
    else
        echo "`date +%Y/%m/%d-%T` [INFO] iltUserSessionsSliverCount configured to: $iltUserSessionsSliverCount"
    fi

    if [ -z "$iltMaxNetworkErrorSimulations" ];
    then
        let iltMaxNetworkErrorSimulations=0
    fi
}

if [ -z "$2" ];
then
    cmd="$1"
    profileDir="."
else
    cmd="$2"
    profileDir="$srcDir/$1"
fi

if [ ! -f "$profileDir/ilt.cfg" ];
then
    echo "$profileDir/ilt.cfg not found, check usage."
    exit 1
fi

# now we can execute one of the supported commands
case "$cmd" in
dotest)
    ;;
checkenvcores|checkenvready|checkenviptables|checkiltstatus|checkiltstatusstage|checkinservicestate|checkinservicestatestage|checkpsudrain|checkpsudrainstage|checkpsustate|checkpsustatestartup|checkpsustatepreshutdown|checkrunning|deployclient|deployserver|deploymockservice|genclientconfig|genserverconfig|makeclient|makeserver|repopulatedb|rerunilt|restartserver|restartserverafterdelay|runilt|runiltwithzdt|runiltnozdt|runiltprep|startclient|startserver|stopall|killall|stopclient|stopmonitors|stopserver|stopmockservice|wipecores|wipedocker|wipelogs|wipeall|wipeboxes|teardownnetworkerrorsimulations|setupnetworkerrorsimulations|startnetworkerrorsimulation|stopnetworkerrorsimulation|getmmmetrics|setupssh)
    ;;
*)
    echo "Usage: $0 [<profileDir>] <command>"
    echo "(profileDir is optional when $0 invoked from profiles/<profileDir>)"
    echo "Commands:"
    echo "  checkenvcores"
    echo "  checkenvready"
    echo "  checkenviptables"
    echo "  checkiltstatus"
    echo "  checkiltstatusstage"
    echo "  checkinservicestate"
    echo "  checkinservicestatestage"
    echo "  checkpsudrain"
    echo "  checkpsudrainstage"
    echo "  checkpsustate"
    echo "  checkpsustatestartup"
    echo "  checkpsustatepreshutdown"
    echo "  checkrunning"
    echo "  deployclient"
    echo "  deployserver"
    echo "  deploymockservice"
    echo "  genclientconfig"
    echo "  genserverconfig"
    echo "  makeclient"
    echo "  makeserver"
    echo "  repopulatedb"
    echo "  rerunilt"
    echo "  restartserver"
    echo "  restartserverafterdelay"
    echo "  runilt"
    echo "  runiltwithzdt"
    echo "  runiltnozdt"
    echo "  runiltprep"
    echo "  startclient"
    echo "  startserver"
    echo "  stopall"
    echo "  killall"
    echo "  stopclient"
    echo "  stopmonitors"
    echo "  stopserver"
    echo "  stopmockservice"
    echo "  wipecores"
    echo "  wipedocker"
    echo "  wipelogs"
    echo "  wipeall"
    echo "  wipeboxes"
    echo "  teardownnetworkerrorsimulations"
    echo "  setupnetworkerrorsimulations"
    echo "  startnetworkerrorsimulation"
    echo "  stopnetworkerrorsimulation"
    echo "  getmmmetrics"
    echo "  setupssh"
    exit 1
esac

source "$profileDir/ilt.cfg"
configDir=$blazeDir
if [ -n "$datacenterProxy" ] && [ $datacenterProxy == $(hostname) ];
then
    configDir="$HOME/blaze"
    _genconfigdir
fi

if [ $iltRestart -ne 0 ] && [ $iltClientUseRedirector -eq 0 ];
then
    iltClientUseRedirector=1
    echo "`date +%Y/%m/%d-%T` [INFO] ILT run will include cluster restart, so overriding (enabling) iltClientUseRedirector setting"
fi

if [ $iltOIExport -ne 0 ] && [ $iltGraphiteExport -ne 0 ];
then
    iltGraphiteExport=0
    echo "`date +%Y/%m/%d-%T` [INFO] OI metrics exporting is enabled, so overriding (disabling) iltGraphiteExport setting"
fi

# build client and server host arrays
buildhostarrays

eval "$cmd"

