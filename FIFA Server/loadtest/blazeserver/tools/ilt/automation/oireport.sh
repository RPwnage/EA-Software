#/bin/bash 


# Redis/Torch metrics are still fetched from Graphite
GRAPHITE_SERVER_FOR_OTHER="eadp.gos.graphite-web.bio-iad.ea.com"
GRAPHITE_PREFIX="eadp.gos." # used in <prefix>blaze.<env>, <prefix>redis.<env>, <prefix>torch.<env>, etc.

# Metrics graphs are obtained via Grafana's rendered image API. Note that a large timeout is needed for the graph images to fully render.
GRAFANA_SERVER="https://gs-shared-graphs.darkside.ea.com"
GRAFANA_ORG_ID="1"
GRAFANA_DASHBOARD="blaze-refactored-metrics-ent"
GRAFANA_API_KEY="eyJrIjoibXRuNFByV0NoS09ndlh3QTY1WmFRNGtXNFJXUzR6SWEiLCJuIjoiZ3MtaWx0cmVwb3J0LWdlbmVyYXRvciIsImlkIjoxfQ=="
GRAFANA_TIMEOUT="999999"
PANEL_ID_ACTIVE_GAMES="23"
PANEL_ID_ACTIVE_PLAYERS="24"
PANEL_ID_CPU="21"
PANEL_ID_FG_SESSIONS="25"
PANEL_ID_GAME_LISTS="26"
PANEL_ID_MEM="22"

relative_script_path=$(dirname ${BASH_SOURCE[0]})
fsv_tools_path="$relative_script_path/../../fetchstatusvariable/"

source "$fsv_tools_path/common"
imgOpts=()
i=0
while [ $i -lt ${#curlOpts[@]} ]; do
  opt=${curlOpts[$i]}
  imgOpts+=("$opt")
  if [ "$opt" == "--max-time" ]; then
    imgOpts+=($GRAFANA_TIMEOUT)
    i=$(($i+1))
  fi
  i=$(($i+1))
done

sshOpts=(-o BatchMode=yes -o StrictHostKeyChecking=no)

function getinstancecounts {
  local oldIFS servers server instances

  servers="$1"

  oldIFS=$IFS
  IFS=$'\n'

  instances=$(echo "${servers[*]}" | awk '{ print $1 }' | sed "s/[0-9]*//g" | sort -uk1,1)
  export report_clusterconfig=""
  for instance in $instances
  do
    report_clusterconfig="$report_clusterconfig""<tr><td> "$instance"</td><td>"$(echo "${servers[*]}" | grep $instance | wc -l)"</td></tr>"$'\n'
  done

  IFS=$oldIFS
}

function getdeploymentinfo {
  local oldIFS servers server instances hosts host filename

  servers="$1"

  oldIFS=$IFS
  IFS=$'\n'

  instances=$(echo "${servers[*]}" | awk '{ print $1 }' | sed "s/[0-9]*//g" | sort -uk1,1)
  hosts=$(echo "${servers[*]}" | awk '{ print $2 }' | sort -uk1,1)
  export report_instances=""
  for instance in $instances
  do
    report_instances=$report_instances"<th>"$instance"</th>"
  done

  for host in $hosts
  do
    filename=/tmp/rep_$host"_"$report_compact_rundate".txt"
    ssh ${sshOpts[*]} $host 'echo "<td>" $(uname -ri) "</td>"; free | grep "Mem:" | awk '\''{ print "<td>" $2/1024 " MB (" int($4*100/$2) "%) </td>" }'\''; free | grep "Swap:" | awk '\''{ print "<td>" int($3*100/$2) "%" }'\''; echo "</td><td>" $(cat /proc/cpuinfo | grep "processor" | wc -l) "@" $(cat /proc/cpuinfo | grep "MHz" | sort -u | awk '\''{ print $4 }'\'') "Mhz" $(cat /proc/cpuinfo | grep "cache size" | sort -u | awk '\''{ print "(" $4 " " $5 ")</td>" }'\'')' >$filename &
  done

  wait

  export report_instancecounts=""
  for host in $hosts
  do
    report_instancecounts="$report_instancecounts""<tr><td>"$(host $host | awk '{ print substr($5, 1, length($5)-1) }')"</td>"
    report_instancecounts="$report_instancecounts"$(cat /tmp/rep_$host"_"$report_compact_rundate".txt")
    rm -f /tmp/rep_$host"_"$report_compact_rundate".txt"
    for instance in $instances
    do
      report_instancecounts="$report_instancecounts""<td>"$(echo "${servers[*]}" | grep $host | grep $instance | wc -l)"</td>"
    done
    report_instancecounts="$report_instancecounts""</tr>"$'\n'
  done

  IFS=$oldIFS
}

function getredisinfo {
  local oldIFS time_from time_until servers server addr clusters cluster hosts host hostabbr hostgraphtag clusterhosts ports filename relfilename url

  servers="$1"
  time_from="$2"
  time_until="$3"

  oldIFS=$IFS
  IFS=$'\n'

  filename=/tmp/redisinfo_$$.txt

  for server in ${servers[*]}
  do
    addr=$(echo $server | awk '{ print $2 }')
    curl ${curlOpts[*]} "http://$addr/blazecontroller/fetchComponentConfiguration?feat|0=framework" | xsltproc $fsv_tools_path/getredisconfig.xslt - | grep -v xml > $filename &

    # just need/want to use any arbitrary host
    break
  done

  wait

  servers=$(cat $filename)
  rm -f $filename

  clusters=$(echo "${servers[*]}" | awk '{ print $1 }' | sed "s/[0-9]*//g" | sort -uk1,1)
  hosts=$(echo "${servers[*]}" | awk '{ print $2 }' | sed "s/:[0-9]*//g" | sort -uk1,1)

  export report_redisclusters=""
  for cluster in $clusters
  do
    report_redisclusters=$report_redisclusters"<th>"$cluster"</th>"
  done

  for host in $hosts
  do
    filename=/tmp/rep_$host"_"$report_compact_rundate".txt"
    ssh ${sshOpts[*]} $host 'echo "<td>" $(uname -ri) "</td>"; free | grep "Mem:" | awk '\''{ print "<td>" $2/1024 " MB (" int($4*100/$2) "%) </td>" }'\''; free | grep "Swap:" | awk '\''{ print "<td>" int($3*100/$2) "%" }'\''; echo "</td><td>" $(cat /proc/cpuinfo | grep "processor" | wc -l) "@" $(cat /proc/cpuinfo | grep "MHz" | sort -u | awk '\''{ print $4 }'\'') "Mhz" $(cat /proc/cpuinfo | grep "cache size" | sort -u | awk '\''{ print "(" $4 " " $5 ")</td>" }'\'')' >$filename &
  done

  wait

  export report_redisclustercounts=""
  for host in $hosts
  do
    report_redisclustercounts="$report_redisclustercounts""<tr><td>"$host"</td>"
    report_redisclustercounts="$report_redisclustercounts"$(cat /tmp/rep_$host"_"$report_compact_rundate".txt")
    rm -f /tmp/rep_$host"_"$report_compact_rundate".txt"
    for cluster in $clusters
    do
      report_redisclustercounts="$report_redisclustercounts""<td>"$(echo "${servers[*]}" | grep $host | grep $cluster | wc -l)"</td>"
    done
    report_redisclustercounts="$report_redisclustercounts""</tr>"$'\n'
  done

  export report_redisgraphs=""
  for cluster in $clusters
  do
    report_redisgraphs="$report_redisgraphs""<div><h4>"$cluster"</h4>"$'\n'
    # graphs for each host in the cluster
    clusterhosts=$(echo "${servers[*]}" | grep $cluster | awk '{ print $2 }' | sed "s/:[0-9]*//g" | sort -uk1,1)
    for host in $clusterhosts
    do
      report_redisgraphs="$report_redisgraphs""<div><h5>"$host"</h5>"$'\n'
      ports=$(echo "${servers[*]}" | grep $host | grep $cluster | awk '{ print $2 }' | sed "s/.*://g")
      ports=$(echo "${ports[*]}" | awk -v ORS=, '{ print $1 }' | sed "s/,$//")
      #echo "cluster host '$host' ports:$ports"
      # assuming the first 'part' of the hostname is unique
      hostabbr=$(echo $host | cut -d'.' -f1)
      # replace '.' with '_' for hostnames in graphite tags
      hostgraphtag=$(echo $host | sed "s/\./_/g")

      relfilename=redis_$cluster"_"$hostabbr"_cpu_"$report_compact_rundate".png"
      filename="$param_target_dir/$relfilename"
      url="http://$GRAPHITE_SERVER_FOR_OTHER/render/?width=500&height=300&from="$time_from"&until="$time_until"&title=CPU&target=substr(scale(sumSeriesWithWildcards(nonNegativeDerivative("$GRAPHITE_PREFIX"redis."$param_env"."$param_service_name"."$cluster"."$hostgraphtag".%7B"$ports"%7D.cpu.parent.*)%2C10)%2C0.33333)%2C6%2C8)"
      curl ${curlOpts[*]} "$url" >"$filename" &
      buf="<img src=\"$relfilename\" />"$'\n'"<p><a href='$url'>cpu utilization</a></p>"
      report_redisgraphs="$report_redisgraphs"$'\n'"$buf"

      relfilename=redis_$cluster"_"$hostabbr"_mem_"$report_compact_rundate".png"
      filename="$param_target_dir/$relfilename"
      url="http://$GRAPHITE_SERVER_FOR_OTHER/render/?width=500&height=300&from="$time_from"&until="$time_until"&title=RSS%20Bytes&target=substr("$GRAPHITE_PREFIX"redis."$param_env"."$param_service_name"."$cluster"."$hostgraphtag".%7B"$ports"%7D.memory.internal_view%2C6%2C8)"
      curl ${curlOpts[*]} "$url" >"$filename" &
      buf="<img src=\"$relfilename\" />"$'\n'"<p><a href='$url'>memory utilization</a></p>"
      report_redisgraphs="$report_redisgraphs"$'\n'"$buf"

      relfilename=redis_$cluster"_"$hostabbr"_cmd_"$report_compact_rundate".png"
      filename="$param_target_dir/$relfilename"
      url="http://$GRAPHITE_SERVER_FOR_OTHER/render/?width=500&height=300&from="$time_from"&until="$time_until"&title=Rate%20of%20Commands&target=substr(nonNegativeDerivative("$GRAPHITE_PREFIX"redis."$param_env"."$param_service_name"."$cluster"."$hostgraphtag".%7B"$ports"%7D.process.commands_processed)%2C6%2C8)"
      curl ${curlOpts[*]} "$url" >"$filename" &
      buf="<img src=\"$relfilename\" />"$'\n'"<p><a href='$url'>commands processed</a></p>"
      report_redisgraphs="$report_redisgraphs"$'\n'"$buf"

      relfilename=redis_$cluster"_"$hostabbr"_hit_"$report_compact_rundate".png"
      filename="$param_target_dir/$relfilename"
      url="http://$GRAPHITE_SERVER_FOR_OTHER/render/?width=500&height=300&from="$time_from"&until="$time_until"&title=Keyspace%20Hit%20%25&target=alias(scale(divideSeries(sumSeries("$GRAPHITE_PREFIX"redis."$param_env"."$param_service_name"."$cluster"."$hostgraphtag".%7B"$ports"%7D.keyspace.hits)%2CsumSeries("$GRAPHITE_PREFIX"redis."$param_env"."$param_service_name"."$cluster"."$hostgraphtag".%7B"$ports"%7D.keyspace.*))%2C100)%2C%27average%20hit%20percentage%27)"
      curl ${curlOpts[*]} "$url" >"$filename" &
      buf="<img src=\"$relfilename\" />"$'\n'"<p><a href='$url'>average hit percentage</a></p>"
      report_redisgraphs="$report_redisgraphs"$'\n'"$buf"

      if [ "$debug" = "yes" ]; then
        echo "$url"
      fi

      report_redisgraphs="$report_redisgraphs"$'\n'"</div>"$'\n'
    done
    report_redisgraphs="$report_redisgraphs""</div>"$'\n'
  done

  wait

  IFS=$oldIFS
}

function getdbinfo {
  local oldIFS servers server instances hosts host filename timestamp scr1 scr2

  servers="$1"
  timestamp="$2"
  destdir="$3"

  oldIFS=$IFS
  IFS=$'\n'

  hosts=$(echo "${servers[*]}" | awk '{ print $4 }')
  export report_databases=""
  export report_slowquery=""
  scr1='echo "<td>" $(uname -ri) "</td>"; free | grep "Mem:" | awk '\''{ print "<td>" $2/1024 " MB (" int($4*100/$2) "%) </td>" }'\''; free | grep "Swap:" | awk '\''{ print "<td>" int($3*100/$2) "%" }'\''; echo "</td><td>" $(cat /proc/cpuinfo | grep "processor" | wc -l) "@" $(cat /proc/cpuinfo | grep "MHz" | sort -u | awk '\''{ print $4 }'\'') "Mhz" $(cat /proc/cpuinfo | grep "cache size" | sort -u | awk '\''{ print "(" $4 " " $5 ")</td>" }'\'');' 
  scr2='for file in $(ls /mysqllog/slow_log/*.log); do tailcount=$(tail -n 1000 $file | awk '\''BEGIN { line = 0; inComment=0; tailBack=0; commentStart=0} { line++; if (substr($0, 1, 1) == "#" ) { if ( inComment == 0) { inComment=1; commentStart=line } } else { inComment=0; } if (tailBack == 0 && $1 == "SET" && substr($2,1, 10)=="timestamp=" && int(substr($2,11, 10)) >= '$timestamp' ) { tailBack=commentStart } } END { if (tailBack > 0) { print line-tailBack+1 } else { print 0 } } '\''); tail -n $tailcount $file; done;'

  for host in $hosts
  do
    hwinfofilename=/tmp/rep_$host"_"$report_compact_rundate"_$$.txt"
    slowqfilename=/tmp/repsq_$host"_"$report_compact_rundate"_$$.txt"
    ssh ${sshOpts[*]} $host $scr1 >$hwinfofilename &
    ssh ${sshOpts[*]} $host $scr2 >$slowqfilename &
  done

  wait

  export report_slowqueryfilename="slow_query_log_"$report_compact_rundate"_"$$".txt"
  echo "" >$destdir/$report_slowqueryfilename
  for server in $servers
  do
    host=$(echo "$server" | awk '{ print $4 }')
    report_databases+=$(echo $server | awk '{ print "<tr><td>" $1 "</td><td>" $2 "</td><td>" $3 "</td><td>" $4 "</td>" }')$(cat /tmp/rep_$host"_"$report_compact_rundate"_$$.txt")"</tr>"$'\n'
    rm -f /tmp/rep_$host"_"$report_compact_rundate"_$$.txt"
    cat /tmp/repsq_$host"_"$report_compact_rundate"_$$.txt" >>$destdir/$report_slowqueryfilename
    rm -f /tmp/repsq_$host"_"$report_compact_rundate"_$$.txt"
  done

  export report_slowquery=$(head -n 10 $destdir/$report_slowqueryfilename)
  if [ -n "$report_slowquery" ]; then
    echo "Error: Slow query detected." >>$report_errfile
  else
    report_slowquery="No slow queries detected."
  fi

  IFS=$oldIFS
}

function checkdbrates {
  oldIFS=$IFS

  IFS=$'\n'

  local servers1=$1
  local servers2=$2
  local interval=$3
  local server1 server2 headerstr rate errrate rateflag errrateflag firstfail firstfailerr server_line1 server_line2

  local i=0
  local cnt=0

  headerstr=$(for (( i=0; i<117; i++ )); do echo -n "-"; done)

  echo $headerstr
  printf "Maximum target SQL rate per minute: %s;  Maximum SQL target error rate per minute: %s\n" $targetSqlRatePerPerMinute $targetSqlFailRatePerMinute
  echo $headerstr

  printf "%-20s %-10s %-10s %-50s %10s%1s %10s%1s\n" "Database" "Name" "Type" "Host" "SQL/min" " " "ERR/min" " "
  echo $headerstr

  firstfail="yes"
  firstfailerr="yes"

  for line in $servers1
  do
    server_line1[$cnt]=$line
    let cnt=$cnt+1
  done

  let cnt=0
  for line in $servers2
  do
    server_line2[$cnt]=$line
    let cnt=$cnt+1
  done

  for (( i=0;i<$cnt;i++ ))
  do
    IFS=$' '
    server1=( ${server_line1[$i]} )
    server2=( ${server_line2[$i]} )
    let rate=${server2[4]}-${server1[4]}*60/$interval
    let errrate=${server2[5]}-${server1[5]}*60/$interval
    rateflag=" "
    errrateflag=" "
    if [ $rate -gt $targetSqlRatePerPerMinute ]; then
      rateflag="*"
      if [ "$firstfail" == "yes" ]; then
        firstfail="no"
        echo "Error: SQL rate was exceeded." >>$report_errfile
      fi
    fi
    if [ $errrate -gt $targetSqlFailRatePerMinute ]; then
      errrateflag="*"
      if [ "$firstfailerr" == "yes" ]; then
        firstfailerr="no"
        echo "Error: SQL error rate was exceeded." >>$report_errfile
      fi
    fi
    printf "%-20s %-10s %-10s %-50s %10s%1s %10s%1s\n" ${server1[0]} ${server1[1]} ${server1[2]} ${server1[3]} "$rate" "$rateflag" "$errrate" "$errrateflag"
    IFS=$'\n'
  done

  echo $headerstr

  IFS=$oldIFS
}

function usage {
  echo "usage: $0 service_name environment platform target_psu reference_file run_duration_minutes target_directory [--graphite graphite_addr --grafana-config grafana_config_file --timestamp timestamp --graphite-prefix graphite_prefix_string --rpcrate rate_file1 rate_file2 interval] [--memcpu memcpu_file1 memcpu_file2] [options]"
  echo "argument values:"
  echo "  environment: prod | test | dev"
  echo "  platform: pc | xbl2 | ps3"
  echo "  target_psu: target PSU in thousands"
  echo "  reference_file: path to reference file which contains target values"
  echo "  run_duration_minutes: how long the test has been running in minutes"
  echo "  target_directory: output location of the report"
  echo "  graphite: default is " $GRAPHITE_SERVER_FOR_OTHER
  echo "  graphite-prefix: default is " $GRAPHITE_PREFIX
  echo "  grafana-config: path to bash file containing overrides for:"
  echo "    GRAFANA_SERVER (default "$GRAFANA_SERVER")"
  echo "    GRAFANA_ORG_ID (default "$GRAFANA_ORG_ID") The Grafana organization id"
  echo "    GRAFANA_DASHBOARD (default "$GRAFANA_DASHBOARD") The dashboard from which to fetch rendered graph images"
  echo "    GRAFANA_API_KEY The API key for accessing the grafana dashboard"
  echo "    GRAFANA_TIMEOUT (default "$GRAFANA_TIMEOUT") The timeout, in seconds, for fetching rendered graph images"
  echo "    PANEL_ID_ACTIVE_GAMES Panel on the grafana dashboard for active game counts by instance name (templated by ServiceName)"
  echo "    PANEL_ID_ACTIVE_PLAYERS Panel on the grafana dashboard for active player counts by instance name (templated by ServiceName)"
  echo "    PANEL_ID_CPU Panel on the grafana dashboard for cpu % by instance name (templated by ServiceName and InstanceType)"
  echo "    PANEL_ID_FG_SESSIONS Panel on the grafana dashboard for active find game session counts by instance name (templated by ServiceName)"
  echo "    PANEL_ID_GAME_LISTS Panel on the grafana dashboard for game list counts by instance name (templated by ServiceName)"
  echo "    PANEL_ID_MEM Panel on the grafana dashboard for memory usage by instance name (templated by ServiceName and InstanceType)"
  echo "  timestamp: unix timestamp; overrides end time"
  echo "options: "
  echo "  --noratedif   Do not test RPC rates"
  echo "  --norpcerr    Do not create detailed RPC error report"
  echo "  --nommscenarios Do not report matchmaking scenarios rpcs and metrics. Non-scenarios matchmaking rpcs and metrics are reported instead"
}

param_service_name=$1
param_env=$2
param_plat=$3
param_target_psu=$4
param_reference_file=$5
param_run_duration_min=$6
param_target_dir=$7
param_rate_file1=""
param_rate_file2=""
param_rate_interval=""
param_mem_file1=""
param_mem_file2=""

check_mem_growth="yes"
check_rate_dif="yes"
detailed_rpc_err="yes"
mmscenarios="yes"
debug="no"
timestamp_override=""

args=("$@")

if [ $# -lt 7 ]; then
  usage
  exit 1
fi

memcpu_test="no"

if [ $# -gt 6 ]; then
   for (( i=7;i<$#;i++ )) do
     argval=${args[$i]}
     if [ "$argval" = "--rpcrata" ]; then
       let i=$i+1
       param_rate_file1=${args[$i]}
       let i=$i+1
       param_rate_file2=${args[$i]}
       let i=$i+1
       param_rate_interval=${args[$i]}
       continue
     elif [ "$argval" = "--memcpu" ]; then
       memcpu_test="yes"
       let i=$i+1
       param_memcpu_file1=${args[$i]}
       let i=$i+1
       param_memcpu_file2=${args[$i]}
       continue
     elif [ "$argval" = "--noratedif" ]; then
       check_rate_dif="no"
       continue
     elif [ "$argval" = "--norpcerr" ]; then
       detailed_rpc_err="no"
       continue
     elif [ "$argval" = "--nommscenarios" ]; then
       mmscenarios="no"
       continue
     elif [ "$argval" = "--help" ]; then
       usage
       exit 0
     elif [ "$argval" = "--graphite" ]; then
       let i=$i+1
       GRAPHITE_SERVER_FOR_OTHER=${args[$i]}
       continue
     elif [ "$argval" = "--graphite-prefix" ]; then
       let i=$i+1
       GRAPHITE_PREFIX=${args[$i]}
       continue;
     elif [ "$argval" = "--grafana-config" ]; then
       let i=$i+1
       source ${args[$i]}
       continue
     elif [ "$argval" = "--timestamp" ]; then
       let i=$i+1
       timestamp_override=${args[$i]}
       continue;
     elif [ "$argval" = "--debug" ]; then
       debug="yes"
       continue;
     fi
     
     if [ "${argval:0:1}" = "-" ]; then
       echo "Unknown option: $argval"
       exit 1
     fi

  done
fi

source $param_reference_file

if [ $param_target_psu -lt 1 ]; then
  echo "Need to define variable 'target_psu' (thousands)" >&2
  usage
  exit 1
fi

if [ $reference_psu -lt 1 ]; then
  echo "Reference file needs to define variable 'reference_psu' (in thousands) and it needs to be greater than 0. Check reference file."
  exit 1
fi

if [ ${targetRpcRateFile:0:1} != "/" ]; then
  targetRpcRateFile=$(dirname $param_reference_file)/$targetRpcRateFile
fi

if [ "$check_rate_dif" == "yes" -a ! -f "$targetRpcRateFile" ]; then
  echo "Variable targetRpcRateFile in reference file is '$targetRpcRateFile' and points to file which does not exist."
  exit 1
fi

if [ "$memcpu_test" == "yes" ]; then
  if [ -z "$targetCpuUsagePercent" ]; then
    echo "Missing variable targetCpuUsagePercent in reference file."
    exit 1
  fi
  if [ -z "$targetMemoryGrowthPercent" ]; then
    echo "Missing variable targetMemoryGrowthPercent in reference file."
    exit 1
  fi

  if [ ! -f "$param_memcpu_file1" -o ! -f "$param_memcpu_file2" ]; then
    echo  "Could not find memory/CPU usage files specified after --memcpu option: '$param_memcpu_file1' and/or '$param_memcpu_file2'"
    exit 1
  fi
fi

let targetGMGauge_ACTIVE_GAMES=$refGMGauge_ACTIVE_GAMES*$param_target_psu/$reference_psu
let targetGMGauge_ACTIVE_GAMES_CLIENT_SERVER_DEDICATED=$refGMGauge_ACTIVE_GAMES_CLIENT_SERVER_DEDICATED*$param_target_psu/$reference_psu
let targetGMGauge_ACTIVE_GAMES_PEER_TO_PEER_FULL_MESH=$refGMGauge_ACTIVE_GAMES_PEER_TO_PEER_FULL_MESH*$param_target_psu/$reference_psu
let targetGMGauge_ACTIVE_PLAYERS=$refGMGauge_ACTIVE_PLAYERS*$param_target_psu/$reference_psu
let targetGMGauge_ACTIVE_PLAYERS_CLIENT_SERVER_DEDICATED=$refGMGauge_ACTIVE_PLAYERS_CLIENT_SERVER_DEDICATED*$param_target_psu/$reference_psu
let targetGMGauge_ACTIVE_PLAYERS_PEER_TO_PEER_FULL_MESH=$refGMGauge_ACTIVE_PLAYERS_PEER_TO_PEER_FULL_MESH*$param_target_psu/$reference_psu
let targetSSGaugeGameLists=$refSSGaugeGameLists*$param_target_psu/$reference_psu
let targetSqlRatePerPerMinute=$targetSqlRatePerPerMinute*$param_target_psu/$reference_psu
let targetSqlFailRatePerMinute=$targetSqlFailRatePerMinute*$param_target_psu/$reference_psu

report_template=$(cat $relative_script_path/oireporttemplate.html)
report_errfile=/tmp/report_$$.err

echo -n "" >$report_errfile

export report_rundate=$(date)
export report_compact_rundate=$(date +%Y%m%d_%H%M%S)
export report_target_psu=$param_target_psu
export report_target_profile=$param_reference_file

export report_tlgamemanager=$( \
  $fsv_tools_path/fsv $param_service_name $param_env $param_plat coreMaster \
    --component gamemanager_master \
      +GMGauge_ACTIVE_GAMES%$targetDeviation-$targetGMGauge_ACTIVE_GAMES \
      +GMGauge_ACTIVE_GAMES_CLIENT_SERVER_DEDICATED%$targetDeviation-$targetGMGauge_ACTIVE_GAMES_CLIENT_SERVER_DEDICATED \
      +GMGauge_ACTIVE_GAMES_PEER_TO_PEER_FULL_MESH%$targetDeviation-$targetGMGauge_ACTIVE_GAMES_PEER_TO_PEER_FULL_MESH \
      +GMGauge_ACTIVE_PLAYERS%$targetDeviation-$targetGMGauge_ACTIVE_PLAYERS \
      +GMGauge_ACTIVE_PLAYERS_CLIENT_SERVER_DEDICATED%$targetDeviation-$targetGMGauge_ACTIVE_PLAYERS_CLIENT_SERVER_DEDICATED \
      +GMGauge_ACTIVE_PLAYERS_PEER_TO_PEER_FULL_MESH%$targetDeviation-$targetGMGauge_ACTIVE_PLAYERS_PEER_TO_PEER_FULL_MESH \
   --component usersessions \
      GaugeUS_CLIENT_TYPE_GAMEPLAY_USER%$targetDeviation-$fulltargetpsu \
   1>/tmp/tmp_$$.txt \
   2>>$report_errfile \
   )

export report_version=$(cat /tmp/tmp_$$.txt | grep "Version" | awk '{ print substr($0, 20) }')
export report_sourcelocation=$(cat /tmp/tmp_$$.txt | grep "Source" | awk '{ print substr($0, 20) }')
export report_buildtime=$(cat /tmp/tmp_$$.txt | grep "Build" | awk '{ print substr($0, 20) }')

#check memory/cpu usage
if [ "$memcpu_test" == "yes" ]; then
  memcpu=$(cat $param_memcpu_file1; echo "--- CutHere"; cat $param_memcpu_file2)
  echo "$memcpu" | awk 'BEGIN {doPercent=0} { if ($0 == "--- CutHere") { doPercent=1 } else { cpu[$1]=$2; if (doPercent) mem[$1] = ($3 - mem[$1]) * 100 / mem[$1]; else mem[$1]= $3; } } END { cpucnt=0; memcnt=0;  for (key in cpu) { if (cpu[key] > '$targetCpuUsagePercent') { cpucnt++; cpustr=cpustr" "key" ("cpu[key]"%)"; }  if (mem[key] > '$targetMemoryGrowthPercent') { memcnt++; memstr=memstr" "key" ("int(mem[key])"%)"; } } if (cpucnt > 0) print "Target CPU usage of '$targetCpuUsagePercent'% was exceeded on nodes: " cpustr; if (memcnt > 0) print "Target memory growth of '$targetMemoryGrowthPercent'% was exceeded on nodes: " memstr; }' >>$report_errfile
fi

oldIFS=$IFS
IFS=$'\n'
redirector=$(getredirector $param_env)
serversAndPorts=$(getservers $redirector $param_service_name $param_env $param_plat "-" "httpxml" 1)
servers=$(echo "${serversAndPorts[*]}" | sed "s/:[0-9]*//g")

if [ "$debug" = "yes" ]; then
  snapstart=$(date +%s)
  if [ -n "$timestamp_override" ]; then
    snapstart=$timestamp_override
  fi
  let timestamp=$snapstart-$param_run_duration_min*60

  grafana_time_from=$(($timestamp*1000))
  grafana_time_to=$(($snapstart*1000))
  dashboard_solo_url="dashboard-solo/db/"$GRAFANA_DASHBOARD"?from="$grafana_time_from"&to="$grafana_time_to"&var-service_name="$param_service_name

  graphite_time_from=$(date -d @$timestamp "+%H:%M_%Y%m%d" | sed -e 's/:/\%3A/') # uri-encode timestamp as date
  graphite_time_until=$(date -d @$snapstart "+%H:%M_%Y%m%d" | sed -e 's/:/\%3A/') # uri-encode timestamp as date

  getredisinfo "$serversAndPorts" "$graphite_time_from" "$graphite_time_until"
  exit 0
fi

getinstancecounts "$servers"
getdeploymentinfo "$servers"
IFS=$oldIFS

report_tlgamemanager=$(cat /tmp/tmp_$$.txt | grep -v Build | grep -v Version | grep -v Source)
rm -f /tmp/tmp_$$.txt

export report_tlsearch=$( \
  $fsv_tools_path/fsv $param_service_name $param_env $param_plat searchSlave \
    --component search \
      +SSGaugeGameLists%$targetDeviation-$targetSSGaugeGameLists \
      +SSGaugeSubscriptionGameLists \
      +SSGaugeActiveFGSessions \
   2>>$report_errfile \
   | grep -v Build | grep -v Version | grep -v Source \
  )

if [ "$mmscenarios" = "yes" ]; then
  export report_tlmatchmaker=$( \
    $fsv_tools_path/fsv $param_service_name $param_env $param_plat mmSlave \
      --component matchmaker \
        MMGaugeActiveMatchmakingSession+$targetMMGaugeActiveMatchmakingSession \
        +ScenarioMMTotalMatchmakingSessionStarted \
        +ScenarioMMTotalMatchmakingSessionSuccess \
        +ScenarioMMTotalMatchmakingSuccessDurationMs \
        +ScenarioMMTotalMatchmakingSessionsCreatedNewGames \
        +ScenarioMMTotalMatchmakingSessionsJoinedNewGames \
        +ScenarioMMTotalMatchmakingSessionsJoinedExistingGames \
        +ScenarioMMTotalMatchmakingSessionTimeout \
      --component usersessions \
        GaugeUS_CLIENT_TYPE_GAMEPLAY_USER%$targetDeviation \
      2>>$report_errfile \
      | grep -v Build | grep -v Version | grep -v Source \
    )
else
  export report_tlmatchmaker=$( \
    $fsv_tools_path/fsv $param_service_name $param_env $param_plat mmSlave \
      --component matchmaker \
        MMGaugeActiveMatchmakingSession+$targetMMGaugeActiveMatchmakingSession \
        +MMTotalMatchmakingSessionStarted \
        +MMTotalMatchmakingSessionSuccess \
        +MMTotalMatchmakingSuccessDurationMs \
        +MMTotalMatchmakingSessionsCreatedNewGames \
        +MMTotalMatchmakingSessionsJoinedNewGames \
        +MMTotalMatchmakingSessionsJoinedExistingGames \
        +MMTotalMatchmakingSessionTimeout \
      --component usersessions \
        GaugeUS_CLIENT_TYPE_GAMEPLAY_USER%$targetDeviation \
      2>>$report_errfile \
      | grep -v Build | grep -v Version | grep -v Source \
    )
fi

export report_rpcerrorsfile="rpcerrorsreport_$report_compact_rundate.txt"
$fsv_tools_path/fec $param_service_name $param_env $param_plat coreSlave $targetErrorRate $targetExceptionRpcs --fastfail 2>>$report_errfile 1>/tmp/rpcerrs_$$.txt &

if [ "$detailed_rpc_err" = "yes" ]; then
  $fsv_tools_path/fec $param_service_name $param_env $param_plat - $targetErrorRate $targetExceptionRpcs 2>/dev/null 1>"$param_target_dir/$report_rpcerrorsfile" &
else
  report_rpcerrorsfile="(no file)"
fi

dbservers1="$($fsv_tools_path/fdb $param_service_name $param_env $param_plat - --noheader)"

if [ $check_rate_dif = "yes" ]; then
  export report_rpcratediffile="rpcdiffreport_$report_compact_rundate.txt"
  if [ -z "$param_rate_file1" -o -z "$param_rate_file2" ]; then
    param_rate_file1="/tmp/rpcrate1_$report_compact_rundate.txt"
    param_rate_file2="/tmp/rpcrate2_$report_compact_rundate.txt"

    let time1=$(date +%s)
    $fsv_tools_path/fec $param_service_name $param_env $param_plat - 0 --noinstance --noheader 2>/dev/null 1>"$param_rate_file1"
 
    let time2=$(date +%s)
    let timedif=$time2-$time1
    if [ $timedif -lt 60 ]; then
      let sleeptime=60-$timedif
      sleep $sleeptime   
    fi

    let time2=$(date +%s)
    let timedif=$time2-$time1
    let param_rate_interval=$timedif
    $fsv_tools_path/fec $param_service_name $param_env $param_plat - 0 --noinstance --noheader 2>/dev/null 1>"$param_rate_file2"
  fi

  dbservers2="$($fsv_tools_path/fdb $param_service_name $param_env $param_plat - --noheader)"
  export report_sqlrates=$(checkdbrates "$dbservers1" "$dbservers2" "$timedif")

  let target_psu_to_ref_psu_ratio_percent=$param_target_psu*100/$reference_psu
  export report_rpcrates=$($fsv_tools_path/diffrate "$param_rate_file1" "$param_rate_file2" "$param_rate_interval" "$targetRpcRateFile" $target_psu_to_ref_psu_ratio_percent $targetRpcRateRelaxFactorPercent $targetRpcRateUpperLimitFactorPercent --refonly 2>>"$report_errfile")
   $fsv_tools_path/diffrate "$param_rate_file1" "$param_rate_file2" "$param_rate_interval" "$targetRpcRateFile" $target_psu_to_ref_psu_ratio_percent $targetRpcRateRelaxFactorPercent $targetRpcRateUpperLimitFactorPercent 2>/dev/null 1>"$param_target_dir/$report_rpcratediffile"
   
  if [ "${param_rate_file1:0:4}" = "/tmp" ]; then
    rm $param_rate_file1
    rm $param_rate_file2
  fi
   
else
  export report_rpcratediffile="(no file)"
  export report_rpcrates="(not done)"
  export report_sqlrates="(not done)"
fi

wait

export report_rpcerrors="$(cat /tmp/rpcerrs_$$.txt)"
rm -f /tmp/rpcerrs_$$.txt

snapstart=$(date +%s)
if [ -n "$timestamp_override" ]; then
  snapstart=$timestamp_override
fi
let timestamp=$snapstart-$param_run_duration_min*60
getdbinfo "$dbservers1" "$timestamp" "$param_target_dir"

#get all instances in cluster
#CPU/mem graphs
instances=$($fsv_tools_path/fsv $param_service_name $param_env $param_plat - --noheader --nobuildinfo --component usersessions GaugeUserSessionsOverall | awk '{ print $1 }' | sed 's/[0-9]//g' | sort -u)

grafana_time_from=$(($timestamp*1000))
grafana_time_to=$(($snapstart*1000))
dashboard_common=$GRAFANA_DASHBOARD"?orgId="$GRAFANA_ORG_ID"&from="$grafana_time_from"&to="$grafana_time_to"&var-ServiceName="$param_service_name
dashboard_solo_url="dashboard-solo/db/"$dashboard_common

graphite_time_from=$(date -d @$timestamp "+%H:%M_%Y%m%d" | sed -e 's/:/\%3A/') # uri-encode timestamp as date
graphite_time_until=$(date -d @$snapstart "+%H:%M_%Y%m%d" | sed -e 's/:/\%3A/') # uri-encode timestamp as date

relfilename=$param_service_name"_psu_"$report_compact_rundate".png"
filename="$param_target_dir/$relfilename"
url="http://$GRAPHITE_SERVER_FOR_OTHER/render/?width=750&height=450&from="$graphite_time_from"&until="$graphite_time_until"&title="$param_service_name
url=$url"&target=cactiStyle(alias(color("$GRAPHITE_PREFIX"torch."$param_env"."$param_service_name".Gameplay_Users%2C%277EB26D%27)%2C%27Gameplay%20Users%27))"
url=$url"&target=alias(color("$GRAPHITE_PREFIX"torch."$param_env"."$param_service_name".Dedicated_Servers%2C%27EAB839%27)%2C%27Dedicated%20Servers%27)"
#url=$url"&target=alias(color("$GRAPHITE_PREFIX"torch."$param_env"."$param_service_name".Limited_Gameplay_Users%2C%276ED0E0%27)%2C%27Limited%20Gameplay%20Users%27)"
#url=$url"&target=alias(color("$GRAPHITE_PREFIX"torch."$param_env"."$param_service_name".Tools_Users%2C%27EF843C%27)%2C%27Tools%20Users%27)"
url=$url"&target=alias(color("$GRAPHITE_PREFIX"torch."$param_env"."$param_service_name".Users_in_Dedicated_Server_Game%2C%27BA43A9%27)%2C%27Users%20in%20Dedicated%20Server%20Game%27)"
url=$url"&target=alias(color("$GRAPHITE_PREFIX"torch."$param_env"."$param_service_name".Users_in_Game%2C%27E24D42%27)%2C%27Users%20in%20Game%27)"
#url=$url"&target=alias(color("$GRAPHITE_PREFIX"torch."$param_env"."$param_service_name".Web_Users%2C%271F78C1%27)%2C%27Web%20Users%27)"
curl ${curlOpts[*]} "$url" >"$filename" &
buf="<img src=\"$relfilename\" />"$'\n'"<p><a href='$url'>PSU</a></p>"
export report_psugraph=$'\n'"$buf"

wait

############# Blaze OI Metrics

export report_cpugraphs=""
for instance in $instances 
do
  relfilename=$instance"_cpu_"$report_compact_rundate".png"
  filename="$param_target_dir/$relfilename"
  url_params_common="&panelId="$PANEL_ID_CPU"&var-InstanceType="$instance
  curl ${imgOpts[*]} -H "Authorization: Bearer $GRAFANA_API_KEY" "${GRAFANA_SERVER}/render/${dashboard_solo_url}${url_params_common}&width=500&height=300&timeout=${GRAFANA_TIMEOUT}" >"$filename" &
  buf="<img src=\"$relfilename\" />"$'\n'"<p><a href=\"${GRAFANA_SERVER}/${dashboard_solo_url}${url_params_common}\">$instance cpu utilization</a></p>"
  report_cpugraphs="$report_cpugraphs"$'\n'"$buf"
done

wait

export report_memgraphs=""
for instance in $instances 
do
  relfilename=$instance"_mem_"$report_compact_rundate".png"
  filename="$param_target_dir/$relfilename"
  url_params_common="&panelId="$PANEL_ID_MEM"&var-InstanceType="$instance
  curl ${imgOpts[*]} -H "Authorization: Bearer $GRAFANA_API_KEY" "${GRAFANA_SERVER}/render/${dashboard_solo_url}${url_params_common}&width=500&height=300&timeout=${GRAFANA_TIMEOUT}" >"$filename" &
  buf="<img src=\"$relfilename\" />"$'\n'"<p><a href=\"${GRAFANA_SERVER}/${dashboard_solo_url}${url_params_common}\">$instance memory usage</a></p>"
  report_memgraphs="$report_memgraphs"$'\n'"$buf"
done

wait

# Other: redis
getredisinfo "$serversAndPorts" "$graphite_time_from" "$graphite_time_until"

export report_games_graph="games_"$report_compact_rundate".png"
url=$GRAFANA_SERVER"/render/"$dashboard_solo_url"&width=500&height=300&panelId="$PANEL_ID_ACTIVE_GAMES"&timeout="$GRAFANA_TIMEOUT
export report_games_graph_url=$GRAFANA_SERVER"/"$dashboard_solo_url"&panelId="$PANEL_ID_ACTIVE_GAMES
curl ${imgOpts[*]} -H "Authorization: Bearer $GRAFANA_API_KEY" "${url}" >$param_target_dir/$report_games_graph

export report_players_graph="players_"$report_compact_rundate".png"
url=$GRAFANA_SERVER"/render/"$dashboard_solo_url"&width=500&height=300&panelId="$PANEL_ID_ACTIVE_PLAYERS"&timeout="$GRAFANA_TIMEOUT
export report_players_graph_url=$GRAFANA_SERVER"/"$dashboard_solo_url"&panelId="$PANEL_ID_ACTIVE_PLAYERS
curl ${imgOpts[*]} -H "Authorization: Bearer $GRAFANA_API_KEY" "${url}" >$param_target_dir/$report_players_graph

export report_find_game_graph="find_game_"$report_compact_rundate".png"
url=$GRAFANA_SERVER"/render/"$dashboard_solo_url"&width=500&height=300&panelId="$PANEL_ID_FG_SESSIONS"&timeout="$GRAFANA_TIMEOUT
export report_find_game_graph_url=$GRAFANA_SERVER"/"$dashboard_solo_url"&panelId="$PANEL_ID_FG_SESSIONS
curl ${imgOpts[*]} -H "Authorization: Bearer $GRAFANA_API_KEY" "${url}" >$param_target_dir/$report_find_game_graph

export report_gb_lists_graph="game_lists_"$report_compact_rundate".png"
url=$GRAFANA_SERVER"/render/"$dashboard_solo_url"&width=500&height=300&panelId="$PANEL_ID_GAME_LISTS"&timeout="$GRAFANA_TIMEOUT
export report_gb_lists_graph_url=$GRAFANA_SERVER"/"$dashboard_solo_url"&panelId="$PANEL_ID_GAME_LISTS
curl ${imgOpts[*]} -H "Authorization: Bearer $GRAFANA_API_KEY" "${url}" >$param_target_dir/$report_gb_lists_graph

runerrors="$(cat $report_errfile)"

full_grafana_dashboard_link="<p><a href=\""$GRAFANA_SERVER"/dashboard/db/${dashboard_common}\">Full Grafana Dashboard</a></p>"

IFS=$' '

if [ -z "$runerrors" ]; then
  export report_success="PASS"
  export report_errors="No errors were reported by automation script."$full_grafana_dashboard_link
  exitcode=0
else
  export report_success="FAIL"
  export report_errors=$(echo "$runerrors" | awk '{ print "<li>"substr($0, 8)"</li>" }')$full_grafana_dashboard_link
  exitcode=1
fi

rm -f $report_errfile

report_html_filename=$param_target_dir/report_$report_compact_rundate".html"
bash -c "echo \"$report_template\"" >$report_html_filename
echo $report_html_filename

exit $exitcode


