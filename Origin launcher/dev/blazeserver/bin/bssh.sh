#!/bin/bash

function _bssh_printxslt()
{
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    echo "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">"
    echo "<xsl:param name=\"instance\"/>"
    echo "<xsl:param name=\"servicename\"/>"
    echo "<xsl:param name=\"channel\"/>"
    echo "<xsl:param name=\"protocol\"/>"
    echo "<xsl:param name=\"bindtype\"/> <!-- 0=internal; 1=external -->"
    echo "<xsl:param name=\"addrtype\" select=\"0\"/>"
    echo "<xsl:template match=\"/\">"
    echo "    <xsl:if test=\"\$instance='-'\">"
    echo "      <xsl:apply-templates select=\"serverlist//serverinfodata[servicenames/servicenames=\$servicename]//serverendpointinfo[protocol=\$protocol and channel=\$channel and bindtype=\$bindtype]//serveraddressinfo[type=\$addrtype]//ip\"/>"
    echo "    </xsl:if>"
    echo "    <xsl:if test=\"\$instance!='-'\">"
    echo "      <xsl:apply-templates select=\"serverlist//serverinfodata[servicenames/servicenames=\$servicename]//serverinstance[starts-with(instancename,\$instance)]//serverendpointinfo[protocol=\$protocol and channel=\$channel and bindtype=\$bindtype]//serveraddressinfo[type=\$addrtype]//ip\"/>"
    echo "      <xsl:apply-templates select=\"serverlist//serverinfodata[servicenames/servicenames=\$servicename]//masterinstance[starts-with(instancename,\$instance)]//serverendpointinfo[protocol=\$protocol and channel=\$channel and bindtype=\$bindtype]//serveraddressinfo[type=\$addrtype]//ip\"/>"
    echo "    </xsl:if>"
    echo "</xsl:template>"
    echo "<xsl:template match=\"ip\">"
    echo "    <xsl:value-of select=\"../../../../../../../instancename/.\" />"
    echo "    <xsl:text> </xsl:text>"
    echo "    <xsl:value-of select=\".\"/>"
    echo "    <xsl:text> </xsl:text>"
    echo "    <xsl:value-of select=\"../port\"/>"
    echo "    <xsl:text> </xsl:text>"
    echo "    <xsl:value-of select=\"../../../../../../../inservice/.\"/>"
    echo "    <xsl:text>&#xa;</xsl:text>"
    echo "  </xsl:template>"
    echo "</xsl:stylesheet>"
}

function bssh()
{
    shopt -s extglob

    local BSSH_PLATFORMS='@(pc|ps3|ps4|xbl2|xone)'
    
    local BSSH_INSTANCENAME=""
    local BSSH_SERVICENAME=""
    local BSSH_PLATFORM=""
    local BSSH_ENVIRONMENT=""
    local BSSH_HOST_IP=""

    local inputArgs=($@)
    local platformSet=false
    for arg in ${inputArgs[@]}
    do
        case "$arg" in
            dev|test|stress|cert|prod)
                BSSH_ENVIRONMENT=$arg
                ;;
            $BSSH_PLATFORMS)
                BSSH_PLATFORM=$arg
                platformSet=true
                ;;
            *)
                if [[ $arg == *"-"* ]];
                then
                    if [ $platformSet = false ]; then
                        local part
                        local parts=($(echo $arg | tr "-" " "))
                        for part in ${parts[@]}
                        do
                            case "$part" in $BSSH_PLATFORMS) BSSH_PLATFORM="$part" ;; esac
                        done
                    fi
                    BSSH_SERVICENAME=$arg
                else
                    BSSH_INSTANCENAME=$arg
                fi
                ;;
        esac
    done

    #if [ -z "$BSSH_SERVICENAME" ]; then
    #    iltssh "$BSSH_INSTANCENAME"
    #    if [ $? -eq 1 ]; then
    #        echo "Could not determine the Blaze cluster service name.  Please provide the service name of the Blaze cluster (e.g. fifa-2017-ps4 or battlefield-1-xone)"
    #    fi
    #    return 1
    #fi

    if [ -z "$BSSH_ENVIRONMENT" ]; then
        echo "Could not determine the Blaze cluster environment.  Please specify one of the following (dev, test, stress, cert, prod)"
        return 1
    fi
    if [ -z "$BSSH_SERVICENAME" ]; then
        echo "Could not determine the Blaze cluster service name.  Please provide the service name of the Blaze cluster (e.g. fifa-2017-ps4 or battlefield-1-xone)"
        return 1
    fi
    #if [ -z "$BSSH_PLATFORM" ]; then
    #    echo "Could not determine the Blaze cluster platform.  Please specify one of the following (pc, ps3, ps4, xbl2, xone)"
    #    return 1
    #fi
    
    local redirector=""
    case "$BSSH_ENVIRONMENT" in
        prod)   redirector="tools.internal.gosredirector.ea.com:42125" ;;
        stress) redirector="web.internal.gosredirector.stest.ea.com:42125" ;;
        test)   redirector="web.internal.gosredirector.stest.ea.com:42125" ;;
        dev)    redirector="internal.gosredirector.online.ea.com:42125" ;;
        *)
            echo "Error: Unknown environment: '$param_env'. Environment must be one of: prod test dev" >&2
            return 1
        ;;
    esac

    echo "Fetching server list for $BSSH_SERVICENAME in the $BSSH_ENVIRONMENT environment (rdir=${redirector}, platform=${BSSH_PLATFORM})..."

    local rc=0

    local serverListXml=$(curl --compressed --silent --max-time 60 "http://$redirector/redirector/getServerList?name=$BSSH_SERVICENAME&env=$BSSH_ENVIRONMENT")
    rc=$?
    if [ $rc -ne 0 ]; then
        echoerr "Failed to get serverlist name=$BSSH_SERVICENAME&env=$BSSH_ENVIRONMENT&plat=$BSSH_PLATFORM from $redirector, curl err=$rc."
        return 1
    fi

    local xsltFilename="/tmp/getserverlist_result_xslt.pid$$"
    _bssh_printxslt > $xsltFilename

    # run the results of the getServerList call through the XSLT file
    oldIFS=$IFS
    IFS=$'\n'
    local serverList=($(echo ${serverListXml} | xsltproc --stringparam bindtype 1 --stringparam addrtype 0 --stringparam channel tcp --stringparam instance "-" --stringparam servicename "$BSSH_SERVICENAME" --stringparam protocol fire2 $xsltFilename - | grep -v xml))
    rc=$? # result from the grep command.
    IFS=$oldIFS

    rm $xsltFilename

    if [ $rc -ne 0 ]; then
        echoerr "Failed to parse serverlist result, parse err=$rc."
        return 1
    fi

    for ((i=0; i<${#serverList[@]}; i++)); do
        local instanceInfo=(${serverList[$i]});

        if [[ "$BSSH_INSTANCENAME" = ${instanceInfo[0]} ]]; then
            BSSH_HOST_IP=$(printf "%08x\n" ${instanceInfo[1]} | awk '{ print strtonum("0x" substr($1, 1, 2))"."strtonum("0x" substr($1, 3, 2))"."strtonum("0x" substr($1, 5, 2))"."strtonum("0x" substr($1, 7, 2)) }')
            echo "Found $BSSH_INSTANCENAME in $BSSH_SERVICENAME ($BSSH_ENVIRONMENT) @$BSSH_HOST_IP.  Connecting..."
            ssh -t -o BatchMode=yes -o StrictHostKeyChecking=no -o ServerAliveInterval=15 "$BSSH_HOST_IP"
            return 0
        fi
    done

    echo "Could not determine the host address using the following information:"
    echo "  Instance Name: $BSSH_INSTANCENAME"
    echo "  Service Name:  $BSSH_SERVICENAME"
    echo "  Environment:   $BSSH_ENVIRONMENT"
    echo "Available instances:"

    local i
    for ((i=0; i<${#serverList[@]}; i++)); do
        local instanceInfo=(${serverList[$i]});

        local instanceName=${instanceInfo[0]}
        local instanceIp=$(printf "%08x\n" ${instanceInfo[1]} | awk '{ print strtonum("0x" substr($1, 1, 2))"."strtonum("0x" substr($1, 3, 2))"."strtonum("0x" substr($1, 5, 2))"."strtonum("0x" substr($1, 7, 2)) }')
        local instanceInService=""
        if [ ${instanceInfo[3]} = "1" ]; then
            instanceInService="IN-SERVICE"
        fi
        printf "  %-15s %-18s %s\\n" "$instanceName" "$instanceIp" "$instanceInService"
    done

    return 1
}

function iltssh()
{
    if ! [ -f ./ilt.cfg ]; then
        echo "Cannot find ./ilt.cfg.  Please run this command in one of the ilt profile directories."
        return 1
    fi

    shopt -s extglob
    
    #echo "...BEGIN iltssh"
    unset masters
    unset slaves
    
    source ./ilt.cfg
    
    # collect all the hosts and instance types
    local allConfigs="$configMaster:configMaster:1"
    #local allTypes="configMaster"

    for master in ${masters[@]}
    do
        allConfigs+=($master)
        # convert row to array of elements
        local masterArr=($(echo "$master" | tr ":" " "))
        local masterType=${masterArr[1]}
        # only add unique elements
        #case "${allTypes[@]}" in *"$masterType"*) ;; *)
        #    allTypes+=($masterType) ;;
        #esac
    done
    
    #echo MASTERS
    #for master in ${masters[@]}; do echo $master; done
    #echo

    for slave in ${slaves[@]}
    do
        allConfigs+=($slave)
        # convert row to array of elements
        local slaveArr=($(echo "$slave" | tr ":" " "))
        local slaveType=${slaveArr[1]}
        # only add unique elements
        #case "${allTypes[@]}" in *"$slaveType"*) ;; *)
        #    allTypes+=($slaveType) ;;
        #esac
    done
    
    #echo "SLAVES"
    #for slave in ${slaves[@]}; do echo $slave; done
    #echo
    
    local oldIFS=$IFS
    IFS=$'\n'
    # sort my lists
    allConfigs=($(for each in ${allConfigs[@]}; do echo $each; done | sort))
    #allTypes=($(for each in ${allTypes[@]}; do echo $each; done | sort -u))
    IFS=$oldIFS
    
    #echo "CONFIGS"
    #for config in ${allConfigs[@]}; do echo $config; done
    #echo
    
    #echo "TYPES"
    #for type in ${allTypes[@]}; do echo $type; done
    #echo
    
    #unset allHosts
    #for config in ${allConfigs[@]}
    #do
    #    local configArr=($(echo "$config" | tr ":" " "))
    #    local configHost=${configArr[0]}
    #    case "${allHosts[@]}" in *"$configHost"*) ;; *)
    #        allHosts+=($configHost) ;;
    #    esac
    #done     
    
    #echo "HOSTS"
    #for host in ${allHosts[@]}; do echo $host; done
    #echo

    instanceToHostArr=()
    local startid=0
    # setup each host (configs should be grouped by host)
    for config in ${allConfigs[@]}
    do
        # convert row to array of elements
        local configArr=($(echo "$config" | tr ":" " "))
        local configHost=${configArr[0]}
        local configType=${configArr[1]}
        local configCount=${configArr[2]}
        #echo "Config: $config"
        #echo "Host: $configHost"
        #echo "Type: $configType"
        #echo "Count: $configCount"
        #sleep 1
        
        local firstId lastId
        let lastId=$startid+$configCount-1
        for (( firstId=$startid; firstId<=$lastId; firstId++ )); do
            instanceToHostArr+=("$configType$firstId $configHost")
        done
        let startid+=configCount
        let index+=1
    done
    
    #echo "INSTANCES"
    #for each in ${instanceToHostArr[@]}; do echo $each; done
    #echo

    local i
    if [ $1 = "listinstances" ]; then
        for i in `seq 1 ${#instanceToHostArr[@]}`; do
            local instanceToHost=( ${instanceToHostArr[i-1]} )
            printf "${instanceToHost[0]} "
        done
        return 0
    else
        for i in `seq 1 ${#instanceToHostArr[@]}`; do
            local instanceToHost=( ${instanceToHostArr[i-1]} )
            if [ ${instanceToHost[0]} = "$1" ]; then
                local foundHost=${instanceToHost[1]}
                echo "Connecting to '$foundHost' as user '${deployUser}'"
                ssh -t -o BatchMode=yes -o StrictHostKeyChecking=no -o ServerAliveInterval=15 "$foundHost" "sudo su - ${deployUser}"
                return $?
            fi
        done
        
        echo "Instance name not found.  The available instances are:"
        for i in `seq 1 ${#instanceToHostArr[@]}`; do
            local instanceToHost=( ${instanceToHostArr[i-1]} )
            echo "${instanceToHost[0]}"
        done
    fi
    
    return 0
}

_iltssh()
{
    COMPREPLY=( $(compgen -W '$(iltssh listinstances)' -- ${COMP_WORDS[COMP_CWORD]}) )
    return 0
}
complete -F _iltssh iltssh

_iltctrl_sh()
{
    local iltctrlCommands="checkenvcores checkenvready checkenviptables checkiltstatus checkiltstatusstage checkinservicestate checkinservicestatestage checkpsustate checkpsustatestagestartup checkpsustatestageshutdown checkrunning deployclient deployserver deploymockservice genclientconfig genserverconfig makeclient makeserver repopulatedb rerunilt restartserver runilt runiltwithzdt runiltnozdt startclient startserver stopall killall stopclient stopmonitors stopserver stopmockservice wipecores wipelogs wipeall wipeboxes teardownnetworkerrorsimulations setupnetworkerrorsimulations startnetworkerrorsimulation stopnetworkerrorsimulation"
    COMPREPLY=( $(compgen -W '$(echo ${iltctrlCommands})' -- ${COMP_WORDS[COMP_CWORD]}) )
    return 0
}
complete -F _iltctrl_sh ../../iltctrl.sh
