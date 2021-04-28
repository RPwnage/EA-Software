#!/bin/bash

# Set exitcode variable based on exit code of last command
# if exit code was not 0, write given error msg to console
set_exit_code()
{
  exitcode=$?
  if [ $exitcode -ne 0 ]; then
    echo $1
  fi
}

if [ -n "$1" ] && [ "$1" = "--help" ]; then
  echo "Usage: ./run.sh [OPTIONS] [-c <cmd>]"
  echo "Omit [-c <cmd>] to start an interactive bash session."
  echo "The default working directory is the blazeserver folder (two directories above this script) if mounting is enabled, and / otherwise."
  echo "OPTIONS:"
  echo "--img=<image> : the docker image to run - default is the image described in blaze_build/TAG" 
  echo "--redis : start up a local Redis server instance. Optional: use --redis=<port> to specify the redis client port (the default is 30000)"
  echo "--nomount  : disable volume mounting."
  echo "--nop4 : don't set P4-related environment variables (also removes the requirement that the P4 client root is a parent of the current directory)"
  echo "--noninteractive : run this script in noninteractive mode (prevents prompting for package server credentials, and starts docker container without keeping STDIN open)"
  echo "--authonly : authenticate with the package server (even if package server credential store file already exists) and exit (forces --nomount and --nop4)"
  echo "--pkguser and --pkgpasswd : use these options to provide username, domain and password for authenticating with the package server (requires --noninteractive)"
  echo "--asan_dev, --asan_tool, --lsan_dev, --lsan_tool (and related sanitizer options): See https://developer.ea.com/display/blaze/Using+Clang+Sanitizers+in+Blaze "
  echo "-D<option> : add docker run option <option>. Use the long form of the option without the '--' - e.g. --cpu-shares=1 becomes -Dcpu-shares=1."
  echo "Note that, with the exception of workdir, no checking is done to ensure that passed-in options don't conflict with built-in options."
  exit 0
fi

pushd $(dirname $0) >/dev/null

exitcode=0
authonly=0
interactive=1
useropts=""
usercmd=""
startedcmd=0
mount=1
asan_tool=0 
asan_halt_on_error=1 
lsan_tool=0 
p4opt=""
workdir=""
image=""
run_redis=0
redisport=30000
consoleopt="-it"
pkguser=""
pkgpasswd=""
for opt in "$@"
do
case $opt in
  -c)
    startedcmd=1;;
  --pkguser=*)
    if [ $startedcmd -ne 0 ]; then
      usercmd+=" $opt"
    else
      pkguser=${opt#--pkguser=}
    fi
    ;;
  --pkgpasswd=*)
    if [ $startedcmd -ne 0 ]; then
      usercmd+=" $opt"
    else
      pkgpasswd=${opt#--pkgpasswd=}
    fi
    ;;
  --authonly)
    if [ $startedcmd -eq 0 ]; then
      authonly=1
      p4opt="--skipcheck"
    else
      usercmd+=" $opt"
    fi
    ;;
  --img=*)
    if [ $startedcmd -ne 0 ]; then
      usercmd+=" $opt"
    else
      image=${opt#--img=}
    fi
    ;;
  --redis*)
    if [ $startedcmd -eq 0 ]; then
      run_redis=1
      arr=(${opt/=/ })
      if [ -n "${arr[1]}" ]; then
        redisport=${arr[1]}
      fi
    else
      usercmd+=" $opt"
    fi
    ;;
  --nomount)
    if [ $startedcmd -eq 0 ]; then
      mount=0
    else
      usercmd+=" $opt"
    fi
    ;;
  --nop4)
    if [ $startedcmd -eq 0 ]; then
      p4opt="--skipcheck"
    else
      usercmd+=" $opt"
    fi
    ;;
  --noninteractive)
    if [ $startedcmd -eq 0 ]; then
      consoleopt="-t"
      interactive=0
    else
      usercmd+=" $opt"
    fi
    ;;
  --asan_tool)
    if [ $startedcmd -eq 0 ]; then
      asan_tool=1
    else
      usercmd+=" $opt"
    fi
    ;;
   --asan_halt_on_error=*)
    if [ $startedcmd -ne 0 ]; then
      usercmd+=" $opt"
    else
      asan_halt_on_error=${opt#--asan_halt_on_error=}
    fi
    ;;
  --lsan_tool)
    if [ $startedcmd -eq 0 ]; then
      lsan_tool=1
    else
      usercmd+=" $opt"
    fi
    ;;
  -Dworkdir=*)
      workdir=${opt#-Dworkdir=};;
  -D*)
    if [ $startedcmd -eq 0 ]; then
      useropts+=" --${opt#-D}"
    else
      usercmd+=" $opt"
    fi
    ;;
  *)
    if [ $startedcmd -ne 0 ]; then
      usercmd+=" $opt"
    else
      echo "ERROR: Unknown option '$opt'"
      exit 1
    fi
    ;;
esac
done

. ../getuserinfo.sh $USER
if [ $mount -eq 0 ]; then
  . ../getp4info.sh $p4opt
else
  . ../getp4info.sh --warn $p4opt
fi

if [ -z "$image" ]; then
  if [ -f ../../../blaze_build/TAG ]; then
    image=$(cat ../../../blaze_build/TAG)
  else
    echo "ERROR: Cannot determine image to run. Either use the --img option with this script, or make sure that file $blazeRoot/blaze_build/TAG is present."
    exit 1
  fi
fi

if [ $run_redis -eq 1 ]; then
  echo "Starting Redis on port $redisport"
  if [ ! -d $blazeRoot/blazeserver/init ]; then
    mkdir $blazeRoot/blazeserver/init
  fi
  docker run --restart no -dit --net=host --name redis_$redisport redis:5.0 redis-server --port $redisport --cluster-enabled yes
  docker exec -it redis_$redisport redis-cli -p $redisport CLUSTER ADDSLOTS 0 $(seq 16383 | xargs -n16383)

  echo "Generating $blazeRoot/blazeserver/init/redis.cfg"
  echo "main = { nodes = [ \"127.0.0.1:$redisport\" ] }" > $blazeRoot/blazeserver/init/redis.cfg
fi

if [ $authonly -ne 0 ] || [ ! -f $HOME/nant_credstore_keys.dat ]; then
  if [ $interactive -ne 0 ]; then
    echo "ACTION REQUIRED: Enter your credentials for the package server"
    echo -n "Username (email): "
    read pkguser
    echo -n "Password: "
    IFS= read -s pkgpasswd
  fi
  docker run --restart no $consoleopt --rm --net=host -u $devuid:$devgid -e HOME=$HOME -v $packageRoot:$packageRoot -v $HOME:$HOME $image mono $(ls -d $blazeRoot/Framework/*|head -n 1)/bin/eapm.exe credstore -user:$pkguser -password:"$pkgpasswd" -use-package-server
  if [ $authonly -ne 0 ]; then
    exit 0
  fi
fi

opts="$consoleopt --rm --cap-add SYS_PTRACE --net=host -u $devuid:$devgid -e USER=$USER -e HOME=$HOME -v $HOME/nant_credstore_keys.dat:$HOME/nant_credstore_keys.dat -v $HOME/.config/.mono/keypairs:$HOME/.config/.mono/keypairs"

if [ -z "$p4opt" ]; then
  p4 set | grep -oE "[^ ]+" | grep "P4" > p4vars.env

  p4ticket=$(echo $HOME/.p4tickets)
  if [ -f "$p4ticket" ]; then
    opts+=" -v $p4ticket:$p4ticket"
  fi
fi

libpatharr=($(find $blazeRoot/blazeserver/build/blazeserver -type d -name container_lib))
libpath=""
if [ -n "$libpatharr" ]; then
  libpath=${libpatharr[0]}
fi

envfiles=($(ls | grep "\.env$"))
for envfile in "${envfiles[@]}"
do
  opts+=" --env-file $envfile"
done

if [ $mount -ne 0 ] || [ -z "$libpath" ]; then
  # Here's a catch-22: before starting our container, we need to know which package roots to mount, and which LD_LIBRARY_PATH to set.
  # But we need to run nant in order to discover our package roots. So we start up a temporary container (and mount our entire home
  # directory to it) just to look up our package info.
  echo "docker run --restart no $opts -v $packageRoot:$packageRoot -w $blazeRoot/blazeserver $image ./nant getpackageinfo"
  docker run --restart no $opts -v $packageRoot:$packageRoot -w $blazeRoot/blazeserver $image ./nant getpackageinfo
  result=$?
  if [ $mount -ne 0 ]; then
    if [ $result -ne 0 ]; then
      echo "ERROR: Unable to determine package roots"
      exit 1
    fi
    packageroots=($(cat ../packageroots.txt))
    for root in "${packageroots[@]}"
    do
      opts+=" -v $root:$root"
    done
    if [ -f $HOME/.p4config ]; then
      opts+=" -v $HOME/.p4config:$HOME/.p4config"
    fi
    if [ -f $HOME/.gdbinit ]; then
      opts+=" -v $HOME/.gdbinit:$HOME/.gdbinit"
    fi
    if [ -f ../packageroots.txt ]; then
      rm -f ../packageroots.txt
    fi
  fi
  if [ -z "$libpath" ]; then
    if [ $result -ne 0 ]; then
      echo "WARNING: Unable to determine LD_LIBRARY_PATH. If you run the blazeserver in this container, you must either set LD_LIBRARY_PATH manually, or use the runblaze or runserver/runserver_opt scripts"
    else
      libpath=$(cat ../builddir.txt)"/unix64-clang-debug/container_lib"
    fi
    if [ -f ../builddir.txt ]; then
      rm -f ../builddir.txt
    fi
  fi
  
  symbolizer=""
  if [ -f ../llvm-symbolizer.txt ]; then
    symbolizer=$(cat ../llvm-symbolizer.txt)
    rm -f ../llvm-symbolizer.txt
  else
    echo "WARNING: Unable to determine llvm-symbolizer path. Sanitizer symbol resolution won't work."
  fi

  asan_so=""
  if [ -f ../asan_so.txt ]; then
    asan_so=$(cat ../asan_so.txt)
    rm -f ../asan_so.txt
  else
    echo "WARNING: Unable to determine asan so path. asan won't work."
  fi

 # asan/lsan common runtime options
  leak_detection_options="detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=15"
  verbosity_option="verbosity=1"

 # asan runtime options
  asan_halt_on_error_option="halt_on_error=$asan_halt_on_error"
  asan_logging_options="log_path=asan_log:log_exe_name=true"
  asan_suppression_option="suppressions=$blazeRoot/blazeserver/bin/asan/asan_suppressions.supp"
  
 # lsan runtime options
  lsan_logging_options="log_path=lsan_log:log_exe_name=true"
  lsan_suppression_option="suppressions=$blazeRoot/blazeserver/bin/asan/lsan_suppressions.supp"

  if [ $asan_tool -eq 1 ]; then
    opts+=" -e ASAN_SYMBOLIZER_PATH=$symbolizer"
    opts+=" -e ASAN_OPTIONS=verify_asan_link_order=1:$leak_detection_options:$asan_logging_options:$asan_halt_on_error_option:$asan_suppression_option:$verbosity_option"
    opts+=" -e LSAN_OPTIONS=$lsan_suppression_option"
  fi

  if [ $lsan_tool -eq 1 ]; then
    opts+=" -e LSAN_SYMBOLIZER_PATH=$symbolizer"
    opts+=" -e LSAN_OPTIONS=$leak_detection_options:$lsan_logging_options:$lsan_suppression_option:$verbosity_option" #For whatever reason, this is not working right now(output is printed at stdout and not in the file)
  fi
fi


if [ -n "$libpath" ]; then
  opts+=" -e LD_LIBRARY_PATH=$libpath"
fi

if [ -n "$workdir" ]; then
  workdir="-w $workdir"
elif [ $mount -ne 0 ]; then
  workdir="-w $blazeRoot/blazeserver"
fi

if [ -z "$usercmd" ]; then
  echo "docker run --restart no $opts $useropts $workdir $image \"/bin/bash\""
  docker run --restart no $opts $useropts $workdir $image "/bin/bash"
  set_exit_code "ERROR: Docker container returned a non-zero exit code"
else
  echo "docker run --restart no $opts $useropts $workdir $image $usercmd"
  docker run --restart no $opts $useropts $workdir $image $usercmd
  set_exit_code "ERROR: Docker container returned a non-zero exit code"
fi

if [ $run_redis -eq 1 ]; then
  echo "Stopping Redis on port $redisport"
  docker stop redis_$redisport
  docker rm redis_$redisport
fi

exit $exitcode
