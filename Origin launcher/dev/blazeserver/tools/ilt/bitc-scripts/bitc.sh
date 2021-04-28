#!/bin/bash

srcDir=$(dirname $(realpath $0))
stopClient=0
stopServer=0

pushconfigs()
{
    cd ../../../
    local filename="manual-${iltTag}.tar.gz"
    if [ -f $filename ]; then
        rm -f $filename
    fi
    tar -cvzf $filename ./etc ./etc.ilt.sports
    local result=$?
    if [ $result -ne 0 ]; then
        cd $srcDir
        return $result
    fi

    kubectl cp $filename bitc-portal-default-0:/efs/blaze/configs/downloads/ -c portal
    result=$?
    if [ $result -eq 0 ]; then
        local accesstoken=$(_getaccesstoken)
        _grpcurl $accesstoken eadp.bitc.v2.Backend.RegisterLocalConfigSet '{"context":{"project_id":"'$projectid'"}, "branch":"'$branch'", "changelist":"manual-'$iltTag'"}'
        result=$?
        if [ $result -eq 0 ]; then
            _grpcurl $accesstoken eadp.bitc.v2.Blaze.UpdateCluster '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'", "config_version":"manual-'$iltTag'", "recreate_schemas":false}'
            result=$?
        fi
    fi
    rm -f $filename
    cd $srcDir
    return $result
}

_updatetemplate()
{
    local template=$(_getcurrenttemplate)
    for arg in "$@"; do
        local parts=(${arg//=/ })
        template=$(echo $template | jq '.blaze.'${parts[0]}' = "'${parts[1]}'"')
    done
    _grpcurl $(_getaccesstoken) eadp.bitc.v2.Blaze.UpdateCluster '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'", "template":'$(echo $template | tr -d "[:space:]")', "recreate_schemas":false}'
}

pullconfigs()
{
    cd ../../../
    if [ -d bitc_configs ]; then
        rm -rf bitc_configs
    fi
    mkdir bitc_configs
    kubectl cp bitc-portal-default-0:/efs/blaze/configs/blaze/$projectid/test/$servicename bitc_configs -c blazeportal
    local result=$?

    cd $srcDir
    return $result 
}

_getetcdport()
{
  local port0=$(kubectl get svc | grep bitc-etcd-0 | awk '{ print $5 }')
  local port1=(${port0//:/ })
  local port2=${port1[1]}
  port1=(${port2//// })
  echo ${port1[0]}
}

_login()
{
    if [ $1 == "bitc" ]; then
        local resp=$(_grpcurl $(_getaccesstoken) eadp.bitc.v2.Backend.GetECRToken '{"context":{"project_id":"'$projectid'"}, "branch":"'$branch'"}')
        local pass=$(echo $resp | jq -r .ecr_token | base64 -d)
        local arr=(${pass//:/ })
        docker login -p ${arr[1]} -u ${arr[0]} $(echo $resp | jq -r .ecr_proxy_url)
    elif [ $1 == "banyan" ]; then
        docker login -u AWS -p $(aws ecr get-login-password --profile stress --region us-east-1) https://827718865590.dkr.ecr.us-east-1.amazonaws.com
    fi
}

_getaccesstoken()
{
    curl -H "application/x-www-form-urlencoded" --silent -d "client_id=91c57d92-4573-41c9-9fc7-560474ca2597&client_secret=qi.pFMYk2DuE4KU5c2F-.u/?ZeS/RJ8H&grant_type=password&scope=openid email profile&username=svc_gsbuild@ea.com&password=baBoc}~yHx%q1[7E\)nZ" -X POST https://login.microsoftonline.com/cc74fc12-4142-400e-a653-f98bfa4b03ba/oauth2/v2.0/token | jq -r .id_token
}

_grpcurl()
{
    grpcurl -H "Authorization: Bearer $1" -H "x-eadp-mesh: LT" -d "${@:3}" $bitcportal $2
}

_checkclusterstate()
{
    local accesstoken=$2
    if [ -z "$accesstoken" ]; then
        accesstoken=$(_getaccesstoken)
    fi
    local resp=$(_grpcurl $accesstoken eadp.bitc.v2.Blaze.GetClusterDetails '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'"}')
    local state=$(echo $resp | jq -r '.state')
    if [ -z "$state" ]; then
        state="CLUSTER_STATE_NOT_FOUND"
    fi
    echo "`date +%Y/%m/%d-%T` [INFO] cluster $servicename is in state $state"
    if [ $state == "CLUSTER_STATE_UNHEALTHY" ] || [ $state == "CLUSTER_STATE_BUILDING" ] || [ $state == "CLUSTER_STATE_DELETING" ]; then
        return 1
    fi
    if [ $state == "CLUSTER_STATE_NOT_FOUND" ]; then
        if [ $1 == "delete" ]; then
            return 0
        else
            return 1
        fi
    elif [ $1 == "delete" ]; then
        return 1
    fi
    return 0
}

checkinservicestate()
{
    local accesstoken=$1
    if [ -z "$accesstoken" ]; then
        accesstoken=$(_getaccesstoken)
    fi
    local resp=$(_grpcurl $accesstoken eadp.bitc.v2.Blaze.GetClusterDetails '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'"}')
    local template=$(_getcurrenttemplate | jq -r '.blaze')
    local counts=($(echo $template | jq -r '.instance_layout[].count'))
    local states=($(echo $resp | jq -r '.instances[].state'))

    local total=0
    for count in ${counts[@]}; do
        total=$((total+count))
    done

    local inservice=0
    for state in ${states[@]}; do
        if [ $state == "COMPONENT_STATE_INSERVICE" ]; then
            inservice=$((inservice + 1))
        fi
    done

    echo "`date +%Y/%m/%d-%T` [INFO] checkinservicestate $inservice of $total instances are in-service"
    
    if [ $inservice -eq $total ]; then
        echo $template | jq -r '[.instance_layout[].name]|@csv' > ./gen/instancetypes
        echo $template | jq -r '.codecl' > ./gen/imagetag
        return 0
    fi

    return 1 
}

_copysql()
{
    local oldPWD=$PWD
    cd $srcDir/../../../
    local files=($(find $1/ -type f -regex ".*\.sql\|.*\.lua"))
    for file in ${files[@]}; do
        local dir=$(dirname $file)
        if [ ! -d docker/eks/archive/blaze/$dir ]; then
            mkdir -p docker/eks/archive/blaze/$dir
        fi
        cp $file docker/eks/archive/blaze/$dir
    done

    cd $oldPWD
}

buildserver()
{
    cd ../../../docker/eks
    local release=$(_getversion)
    local buildtype=$(_getbuildinfo $buildtarget)
    local build_dir="../../build/blazeserver/$release/$buildtype"
    if [ ! -f $build_dir/bin/blazeserver ]; then
        echo "No blazeserver binary at $build_dir/bin/blazeserver"
        return 1
    fi

    rm -rf archive/*
    mkdir -p archive/blaze/bin
    mkdir archive/blaze/lib
    mkdir archive/blaze/scripts

    cp scripts/* archive/blaze/scripts/
    cp -r ../../bin/asan archive/blaze/bin/
    cp -r ../../bin/dbmig archive/blaze/bin/
    cp -r ../../bin/info-exception archive/blaze/bin/
    cp -r ../../bin/info-core archive/blaze/bin/
    cp -r ../../bin/info-core-opt archive/blaze/bin/
    cp $build_dir/bin/blazeserver archive/blaze/bin/
    
    if [ -n "$(find $build_dir/bin/ -type f -name *.so* | head -n 1)" ]; then
        cp $build_dir/bin/*.so* archive/blaze/lib/    
    fi
    if [ -f $build_dir/bin/llvm-symbolizer ]; then
        cp $build_dir/bin/llvm-symbolizer archive/blaze/bin/
    fi
    cp $build_dir/container_lib/* archive/blaze/lib/
    cp -f archive/blaze/lib/libmariadb.so archive/blaze/lib/libmariadb.so.3

    _copysql component
    _copysql customcomponent
    _copysql framework         

    cd archive/blaze
    tar -zcvf ../blaze.tar.gz . 
    cd ../../
    local baseimage=$(_getbuildinfo baseimage)
    if [ -z "$(docker images -q $baseimage)" ]; then
        local fullimage=$baseimage
        baseimage=${fullimage##*/}
        if [ -z "$(docker images -q $baseimage)" ]; then
            docker pull $fullimage
            if [ $? -ne 0 ]; then
                echo "No available blazeserver base image ($baseimage or $fullimage)"
                return 1
            fi
        fi
    fi
    docker build -t $blazerepo:$iltTag --build-arg BLAZE_TAR_FILE=blaze.tar.gz --build-arg BLAZE_BASE_IMAGE=$baseimage .
    local result=$?
    if [ $result -eq 0 ]; then
        _login bitc
        docker push $blazerepo:$iltTag
        result=$?
    fi
    cd $srcDir
    return $result
}

_getversion()
{
    cat $srcDir/../../../Manifest.xml | grep "<versionName>" | sed -e 's|.*<versionName>\(.*\)|\1|' | sed -e 's|\(.*\)</versionName>.*|\1|'
}

_getbuildinfo()
{
    cat $srcDir/../../../bin/package.cfg | grep "$1 =" | sed -e "s|.*$1 = \(.*\)|\1|"
}

buildclient()
{
    _login bitc
    cd ../../../docker/stress
    local release=$(_getversion)
    local buildtype=$(_getbuildinfo $buildtarget)
    local build_dir="../../build/blazeserver/$release/$buildtype"
    if [ ! -f $build_dir/bin/stress ]; then
        echo "No stress binary at $build_dir/bin/stress"
        return 1
    fi

    rm -rf archive/bin
    mkdir archive/bin
    cp -f ../../etc.ilt.sports/stress/geoip_samples_db.csv archive/init/

    cp $build_dir/bin/stress archive/bin/
    strip archive/bin/stress
    cp $build_dir/container_lib/* archive/bin/
    docker build -t $clientrepo:$servicename .
    local result=$?
    if [ $result -eq 0 ]; then
        _login banyan
        docker push $clientrepo:$servicename
        result=$?
    fi
    cd $srcDir
    return $result
}

startserver()
{
    stopServer=1
    _grpcurl $(_getaccesstoken) eadp.bitc.v2.Blaze.StartCluster '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'", "use_mock_services":'$useMock'}'
}

_genclient()
{
    set -f
    echo "{"
    echo "\"projectId\": $banyanprojectid,"
    echo "\"name\":\"${servicename}-integrated\","
    echo "\"durationInMins\":$durationMins,"
    echo "\"specs\": ["

    local sarr=(${servicename//-/ })
    local platform=${sarr[2]}
    local prefix="${sarr[0]}-${sarr[1]}"
    local etcdport=$1

    local commonParams="--projectid=$projectid --etcdendpoint=http://${etcdhost}:$etcdport -use_stress_login $useStressLogin -DSTRESS_SECURE=$clientSecure"
    if [ $useMock == "true" ]; then
        commonParams+=" -DMOCK_EXTERNAL_SESSIONS=true"
    fi
    commonParams+=" -DPLATFORM=$platform -DSERVICE_NAME_PREFIX=$prefix"

    local oldIFS=$IFS
    local index=1
    local total=$(cat $layout | wc -l)
    local count=1
    while IFS= read -r line
    do
      local arr=($line)
      export testname="${servicename}_${arr[0]}"
      export numConns=${arr[3]}
      export params="$commonParams -DTEST_CONFIG=${arr[2]}"
      export startIndex=$index
      local invIdx=${arr[4]}
      export invId=${inventoryids[$invIdx]}
      local l=${arr[1]}
      export load=$(( psuFactor*l ))
      local data=$(cat spec.tpl)
      if [ $count -eq $total ]; then 
          echo $(eval "echo \"${data}\"")
      else
        echo $(eval "echo \"${data},\"")
      fi
      index=$(( index+load+1 ))
      count=$(( count+1 ))
    done < "$layout"

    IFS=$oldIFS

    echo "]}"
    set +f
}

startclients()
{
    local filename="gen/testdef.json"
    local etcdport=$(_getetcdport)
    nc -w1 $etcdhost $etcdport
    if [ $? -ne 0 ]; then
        echo "`date +%Y/%m/%d-%T` [ERR] etcd endpoint $etcdhost:$etcdport is unreachable. You may need to update the host ip in route53 (in the aws console, account 671536334799) and/or create a new NodePort service (kubectl expose pod bitc-etcd-0 --type=NodePort --port=2379) and update the nodes-${clustername}-etcd security group (in the aws console, account $awsaccount)" 
        return 1
    fi
    _genclient $etcdport > $filename
  
    curl --silent -X POST -H "X-Bnyn-User: $USER@ea.com" -H "accept: application/json" -H "Content-Type: application/json" --data @$filename 'http://banyan.ea.com/testautomation/runtest-service/api/v2/integratedtests/' >gen/testrun.json 
    echo "integratedTestId=$(cat ./gen/testrun.json | jq -r '.id')" >> ./gen/status
    local testids=($(cat ./gen/testrun.json | jq -r '.testIds[]'))
    echo "testids=(${testids[@]})" >> ./gen/status
    local total=${#testids[@]}

    if [ $cleanupClientNodes == "true" ]; then
        stopClient=1
    fi

    while true
    do
        local cur=0
        for id in ${testids[@]}
        do
            local state=$(curl --silent -H "X-Bnyn-User: $USER@ea.com" -H "accept: application/json" "http://banyan.ea.com/testautomation/runtest-service/api/v2/tests/$id" | jq -r .state)
            if [ $state == "ABORTED" ]; then
                echo "`date +%Y/%m/%d-%T` [ERR] Test $id failed to start"
                return 1
            fi
            if [ $state == "RUNNING" ]; then
                cur=$((cur+1))
            fi
        done
        echo "`date +%Y/%m/%d-%T` [INFO] $cur / $total tests are in state RUNNING"
        if [ $cur -eq $total ]; then
            return 0
        fi
        sleep 15
    done
    return 1
}

stopserver()
{
    local forcekill="true"
    if [ -f ./gen/imagetag ]; then
      local imagetag=$(cat ./gen/imagetag)
      local imageinfo=(${imagetag//-/ })
      if [ ${imageinfo[-1]} == "asan" ]; then
        forcekill="false"
      fi
    fi
    _grpcurl $(_getaccesstoken) eadp.bitc.v2.Blaze.StopCluster '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'", "force_kill":'$forcekill'}'
    local result=$?
    sed -i '/^end=/d' ./gen/status
    local timestamp=$(_timestamp)
    echo "end=${timestamp}" >> ./gen/status
    return $result
}

restartserver()
{
    _grpcurl $(_getaccesstoken) eadp.bitc.v2.Blaze.RestartCluster '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'", "resume_previous_restart":false, "instance_types":['$(cat ./gen/instancetypes)']}'
}

_restartafterdelay()
{
    _delay $iltRestartDelayMin
    restartserver
}

_stopafterdelay()
{
    _delay $((durationMins - iltRestartDelayMin))
    stopserver
}

_timestamp()
{
    local timestamp=$(date +%s)
    echo $(($timestamp*1000))
}

_delay()
{
    local sec=$1
    local sleepTimeSec=$((60*sec))
    echo "`date +%Y/%m/%d-%T` [INFO] delaying $(date -u -d @${sleepTimeSec} +%T)"
    sleep $sleepTimeSec
    return 0
}

createcluster()
{
    local arr=(${servicename//-/ })
    local prefix="${arr[0]}-${arr[1]}"
    local postfix="${arr[3]}"
    local accesstoken=$(_getaccesstoken)
    local coderesp=$(_grpcurl $accesstoken eadp.bitc.v2.Branches.ListBuilds '{"context":{"project_id":"'$projectid'"}, "branch":"'$branch'", "type":1,"states":[1]}')
    local configresp=$(_grpcurl $accesstoken eadp.bitc.v2.Branches.ListBuilds '{"context":{"project_id":"'$projectid'"}, "branch":"'$branch'", "type":2,"states":[1]}')
    local codeversion=$(echo $coderesp | jq -r '.builds[0].build_name')
    local configversion=$(echo $configresp | jq -r '.builds[0].build_name')
    local plat=${arr[2]}
    local platform="PLATFORM_${plat^^}"

    _grpcurl $accesstoken eadp.bitc.v2.Blaze.CreateCluster '{"context":{"project_id":"'$projectid'"}, "platform":"'$platform'", "blaze_environment":2, "service_name_prefix":"'$prefix'", "service_name_postfix":"'$postfix'", "branch_name":"'$branch'","template_name":"'$prefix'","code_version":"'$codeversion'","config_version":"'$configversion'"}'
    _checkstatestage "createcluster" $iltCreateClusterTimeoutMin
}

deletecluster()
{
    _grpcurl $(_getaccesstoken) eadp.bitc.v2.Blaze.DeleteCluster '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'"}'
    _checkstatestage "deletecluster" $iltCreateClusterTimeoutMin
}

_validatecluster()
{
    _checkstatestage "createcluster" $iltCreateClusterTimeoutMin
}

checkinservicestatestage()
{
    _checkstatestage "checkinservicestate" $iltInServiceTimeoutMin
}

_checkstatestage()
{
    local accesstoken=$(_getaccesstoken)
    local result
    local sleepTimeSec=10
    local timeoutMin=$2
    local startTime=$(date +%s)
    local finishTime=$(($startTime + 60*$timeoutMin))
    while [ $(date +%s) -lt $finishTime ]
    do
        if [ $1 == "checkinservicestate" ]; then
          checkinservicestate $accesstoken
        elif [ $1 == "createcluster" ]; then
          _checkclusterstate "create" $accesstoken
        else
          _checkclusterstate "delete" $accesstoken
        fi   
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
        echo "`date +%Y/%m/%d-%T` [ERR] $1 (timed out after $(date -u -d @${timeoutTime} +%T))"
    fi
    return $result
}

stopclients()
{
    if [ ! -f ./gen/status ]; then
        echo "`date +%Y/%m/%d-%T` [ERR] stopclients: no saved status found"
        return 1
    fi
 
    source ./gen/status
    local remaining=(${testids[@]})
    while [ ${#remaining[@]} -gt 0 ]
    do
      local temp=(${remaining[@]})
      remaining=()
      for test in ${temp[@]}
      do
          local state=$(curl --silent -H "X-Bnyn-User: $USER@ea.com" -H "accept: application/json" "http://banyan.ea.com/testautomation/runtest-service/api/v2/tests/$test" | jq -r .state)
          if [ $state == "ABORTED" ] || [ $state == "STOPPED" ]; then
              echo "`date +%Y/%m/%d-%T` [INFO] Test $test is $state"
          else
              echo "`date +%Y/%m/%d-%T` [INFO] Waiting for test $test (current state $state)"
              curl --silent -X POST -H "accept: application/json" -H "Content-Type: application/json" --data '{"action":"stop"}' 'http://banyan.ea.com/testautomation/runtest-service/api/v2/tests/'$test >/dev/null
              remaining+=($test)
          fi
      done
      if [ ${#remaining[@]} -eq 0 ]; then
          break
      fi
      sleep 5
    done 
    return 0
}

stopall()
{
    stopclients
    stopserver
}

status()
{
    if [ ! -f ./gen/status ]; then
        echo "No saved status available."
        echo "To check for running blazeservers, go to $bitcgui"
        echo "To check for running clients, go to https://console.ea.com/testautomation/dashboard/${banyanprojectid}?project=${projectname}"
    else
        source ./gen/status
        if [ -n "$integratedTestId" ]; then
            echo "Banyan: https://console.ea.com/testautomation/dashboard/${banyanprojectid}/integrated-tests/${integratedTestId}?project=${projectname}"
        fi
        if [ -z "$end" ]; then
            echo "Grafana Dashboard: ${grafana}&from=${start}&to=now"
        else
            echo "Grafana Dashboard: ${grafana}&from=${start}&to=${end}"
        fi
    fi
}

_initstatus()
{
    if [ -d ./gen ]; then
        rm -rf ./gen
    fi
    mkdir ./gen
    local timestamp=$(_timestamp)
    echo "grafana=\"${grafanaurl}&var-blaze_servicename=${servicename}\"" > ./gen/status
    echo "start=${timestamp}" >> ./gen/status
}

runilt()
{
    local stages=()
    stages+=(_validatecluster)
    stages+=(buildclient)
    stages+=(_initstatus)
    stages+=(startserver)
    stages+=(checkinservicestatestage)
    stages+=(startclients)
    if [ $iltRestartDelayMin -ne 0 ];
    then
        stages+=(_restartafterdelay)
    fi
    stages+=(_stopafterdelay)
    stages+=(status)

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

    exit 0
}

_cleanup()
{
    echo "`date +%Y/%m/%d-%T` [INFO] cleaning up..."

    if [ $stopClient -ne 0 ];
    then
        stopclients
    fi

    if [ $stopServer -ne 0 ];
    then
        stopserver
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

wipelogs()
{
    kubectl exec -ti bitc-portal-default-0 -c portal -- rm -rf /efs/blaze/log/test/$servicename
}

_getcurrenttemplate()
{
    local accesstoken=$(_getaccesstoken)
    local dets=$(_grpcurl $accesstoken eadp.bitc.v2.Blaze.GetClusterDetails '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'"}') 
    local revid=$(echo $dets | jq -r '.blaze_revision')
    local resp=$(_grpcurl $accesstoken eadp.bitc.v2.BlazeTemplates.GetTemplateRevisions '{"context":{"project_id":"'$projectid'"}, "blaze_environment":2, "service_name":"'$servicename'", "section":0, "revisions":["'$revid'"]}') 
    echo $resp | jq -r '.revisions[0].template'
}

applyserver()
{
    _updatetemplate "codecl=$iltTag"
}

case "$1" in
buildclient|buildserver|applyserver|checkinservicestate|checkinservicestatestage|createcluster|deletecluster|pullconfigs|pushconfigs|restartserver|runilt|startclients|startserver|status|stopall|stopclients|stopserver|wipelogs)
    ;;
*)
    echo "Unrecognized command: '$1'. Valid commands are:"
    echo "    buildclient - push a new stress client image to the Banyan registry (does not update stress clients that are already running)"
    echo "    buildserver - build a new blazeserver image and upload it to the BitC registry (use applyserver to apply the image to the current BitC blazeserver)"
    echo "    applyserver - apply a new blazeserver image to the current BitC blazeserver (forces a rolling restart if the server is running)"
    echo "    checkinservicestate - check if all blazeserver instances are in-service"
    echo "    checkinservicestatestage - wait until all blazeserver instances are in-service"
    echo "    createcluster - create the blaze cluster"
    echo "    deletecluster - delete the blaze cluster"
    echo "    pullconfigs - download current BitC blazeserver configs to blazeserver/bitc_configs"
    echo "    pushconfigs - upload local configs (blazeserver/etc and blazeserver/etc.ilt.sports) to the current BitC blazeserver"
    echo "    restartserver - perform a rolling restart of all blaze instance types"
    echo "    runilt - start a full ilt run"
    echo "    startclients - launch the stress clients in Banyan"
    echo "    startserver - launch the blazeserver in BitC"
    echo "    status - print links to the Blaze Grafana dashboard and Banyan ILT report (if available)"
    echo "    stopall - stop (force-kill) the blazeserver and all stress clients"
    echo "    stopclients - stop the stress clients"
    echo "    stopserver - stop (force-kill) the blazeserver"
    echo "    wipelogs - delete all blazeserver log files from EFS (only affects file-based logs; any process/stdout logs will still be available)"
    exit 1
esac

cd $srcDir
source ./ilt.cfg
if [ $servicename == "example" ] || [ -z "$(ls ./servicenames | grep $servicename)" ]; then
    echo "Invalid service name: '$servicename'. Choose a servicename from the list of files in ./servicenames (excluding example)."
    exit 1
fi
source ./servicenames/$servicename
eval "$1"
