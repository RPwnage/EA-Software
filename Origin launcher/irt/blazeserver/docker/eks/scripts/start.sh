#!/bin/bash

ulimit -HSn 250000
ulimit -a

if [ -n "${ASAN_SYMBOLIZER_PATH}" ]; then
  if [ ! -f ${ASAN_SYMBOLIZER_PATH} ]; then
    echo "`date +%Y/%m/%d-%T` [init][asan] ERR Missing ASan symbolizer ${ASAN_SYMBOLIZER_PATH}"
    exit 1
  fi
fi
if [ -n "${ASAN_SO}" ]; then
  if [ ! -f ${ASAN_SO} ]; then
    echo "`date +%Y/%m/%d-%T` [init][asan] ERR Missing ASan so ${ASAN_SO}"
    exit 1
  fi
  if [ -z "$(ldd /app/bin/blazeserver | grep ${ASAN_SO})" ]; then
    echo "`date +%Y/%m/%d-%T` [init][asan] ERR Blazeserver is not linked with defined ASAN_SO (${ASAN_SO})"
    ldd /app/bin/blazeserver
    exit 1
  fi
fi

dbcfg="/app/init/databases.cfg"
if [ -f $dbcfg ]; then
  rm -f $dbcfg
fi

vaultpath=${VAULT_PATH_DB}

# DB_CONFIGS syntax: <config1>=<schema1>,<schema2>;<config2>=<schema3>,<schema4>
# e.g. framework/databases_main.cfg=main,main_pc,main_ps4;framework/databases_stats.cfg=stats,stats_pc,stats_ps4
configs=(${DB_CONFIGS//;/ })
for config in ${configs[@]}
do
  params=(${config//=/ })
  schemastr=${params[1]}
  schemas=(${schemastr//,/ })
  for schema in ${schemas[@]}
  do
    echo "${schema} = {" >> $dbcfg
    echo "#define VAULT_PATH_DB_WRITER \"${vaultpath}/${schema}/writer\"" >> $dbcfg
    echo "#define VAULT_PATH_DB_READER \"${vaultpath}/${schema}/reader\"" >> $dbcfg
    echo "#include \"${params[0]}\"" >> $dbcfg
    echo "}" >> $dbcfg
  done
done

echo "`date +%Y/%m/%d-%T` [init][database] Generated databases.cfg:"
cat $dbcfg

rediscfg="/app/init/redis.cfg"
if [ -f $rediscfg ]; then
  rm -f $rediscfg
fi

# REDIS_CLUSTERS syntax: <cluster1>=<endpoint1>,<endpoint2>;<cluster2>=<endpoint1>,<endpoint2>
# e.g. main=127.0.0.1:30000,127.0.0.1:30001;custom=127.0.0.1:6379
clusters=(${REDIS_CLUSTERS//;/ })
for cluster in ${clusters[@]}
do
  params=(${cluster//=/ })
  echo "${params[0]} = {" >> $rediscfg
  echo "nodes = [ ${params[1]} ]" >> $rediscfg
  echo "}" >> $rediscfg
done

echo "`date +%Y/%m/%d-%T` [init][redis] Generated redis.cfg:"
cat $rediscfg

INTERNAL_PORTS=""
EXTERNAL_PORTS=""
i=0
while [ $i -lt $NUM_INTERNAL_PORTS ]
do
  url="localhost:11021/ports/$POD_NAME/internal$i?pool=internal"
  echo "`date +%Y/%m/%d-%T`[init][DPA] Calling url: $url"
  port=$(curl -s $url | grep -Po '(?<=PortNumber":)[0-9]+')

  if [ -n "$INTERNAL_PORTS" ]; then
    INTERNAL_PORTS="${INTERNAL_PORTS},$port"
  else
    INTERNAL_PORTS=$port
  fi
  i=$(($i+1))
done

j=0
while [ $j -lt $NUM_EXTERNAL_PORTS ]
do
  url="localhost:11021/ports/$POD_NAME/external$j?pool=external"
  echo "`date +%Y/%m/%d-%T`[init][DPA] Calling url: $url"
  port=$(curl -s $url | grep -Po '(?<=PortNumber":)[0-9]+')

  if [ -n "$EXTERNAL_PORTS" ]; then
    EXTERNAL_PORTS="${EXTERNAL_PORTS},$port"
  else
    EXTERNAL_PORTS=$port
  fi
  j=$(($j+1))
done

if [ -z "$INTERNAL_PORTS" ]; then
  reason="DPA failure"
  if [ $NUM_INTERNAL_PORTS -lt 1 ]; then
    reason="misconfigured blaze cluster template"
  fi
  echo "`date +%Y/%m/%d-%T`[init][DPA] ERR No internal ports ($reason); aborting"
  exit 1
fi
allports="--internalports $INTERNAL_PORTS"
if [ -n "$EXTERNAL_PORTS" ]; then
  allports="$allports --externalports $EXTERNAL_PORTS"
fi

noncollocated=""
if [ -n "$INSTANCE_TYPE" ]; then
  podnamearr=(${POD_NAME//-/ })
  instancename="${INSTANCE_TYPE}${podnamearr[-1]}"
  noncollocated=" --type $INSTANCE_TYPE --name $instancename --logname $instancename"
fi
if [ -n "${ASAN_LOG_PATH_OPTION}" ]; then
  export ASAN_OPTIONS="${ASAN_OPTIONS}:${ASAN_LOG_PATH_OPTION}/asan_${instancename}_$(date -u +%Y%m%d_%H%M%S)"
fi

cmd="/app/bin/blazeserver $noncollocated $allports -DBASE_PORT=0 -DUSING_BITC=1"
defs=($(set | grep ^BLAZEDEF_))
for val in ${defs[@]}; do
  cmd+=" -D${val#BLAZEDEF_}"
done
if [ -n "$BLAZE_BOOT_FILE_OVERRIDE" ]; then
  cmd+=" --bootFileOverride $BLAZE_BOOT_FILE_OVERRIDE"
fi
cmd+=" --logdir $BLAZE_LOG_DIR --argfile $BLAZE_ARG_FILE $BLAZE_BOOT_FILE --dbdestructive"

echo "`date +%Y/%m/%d-%T`[init][blaze] Executing: $cmd"
exec $cmd    

