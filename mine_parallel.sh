#!/bin/bash

# check required commands
for cmd in git make; do
  if ! [ -x "$(command -v $cmd)" ]; then
    echo "Error: $cmd is not installed" >&2
    exit 1
  fi
done

# common settings
log_level=error
check_interval=172800
min_changes=20
min_cpus=4
branch=$(git rev-parse --abbrev-ref HEAD)

# get the number of cpus
if [[ "$OSTYPE" == "darwin"* ]]; then
  num_cpus=$(sysctl -n hw.ncpu)
else
  num_cpus=$(cat /proc/cpuinfo | grep processor | wc -l)
fi

# check the minimum number of cpus
if [ "$num_cpus" -lt "$min_cpus" ]; then
  echo "Need at least ${min_cpus} CPUs, but only ${num_cpus} were found"
  exit 1
fi

function stop_miners() {
  echo "Stopping miners"
  killall loda > /dev/null
}

function abort_miners() {
  stop_miners
  exit 1
}

function run_loda {
  echo "loda $@"
  ./loda $@ &
}

function start_miners() {
  ./loda update
  if ! ./loda test; then
    echo "Error during tests"
    exit 1
  fi
  echo "Start mining"

  # configuration
  local l="-l ${log_level}"
  templates=T01
  num_vars=4
  indirect=false
  if [ "$num_cpus" -ge 8 ]; then
    templates="T01 T02"
    indirect=true
  fi
  if [ "$num_cpus" -ge 12 ]; then
    num_vars="4 6"
  fi
  if [ "$num_cpus" -ge 16 ]; then
    num_vars="4 6 8"
  fi
  if [ "$num_cpus" -ge 20 ]; then
    num_vars="4 6 8 10"
  fi
  if [ "$num_cpus" -ge 24 ]; then
    num_vars="2 4 6 8 10"
  fi

  # start parameterized miners
  for n in ${num_vars}; do
    for x in "" "-x"; do
      args="-n $n -p ${n}0 -a cd $x $l"
      # instantiate templates w/o loops
      for t in ${templates}; do
        run_loda mine $args -o ^l -e programs/templates/${t}.asm $@
      done
      # no templates but w/ loops
      run_loda mine $args $@
    done
  done

  # indirect memory access
  if [ "$indirect" = "true" ]; then
    run_loda mine -n 6 -p 60 -a cdi $l $@
  fi

  # maintenance
  if [ "$branch" = "master" ]; then
    run_loda maintain
  fi
}

function push_updates {
  if [ ! -d "programs" ]; then
    echo "programs folder not found; aborting"
    exit 1
  fi
  num_changes=$(git status programs -s | wc -l)
  if [ "$num_changes" -ge "$min_changes" ]; then
  	stop_miners
  	if [ "$branch" = "master" ]; then
      ./make_charts.sh
    fi 
    echo "Pushing updates"
    git pull
    git add programs
    if [ "$branch" = "master" ]; then
      git add stats README.md
    fi
    git commit -m "updated $num_changes programs"
    git push
    echo "Rebuilding loda"
    pushd src && make clean && make && popd
  fi
  ps -A > /tmp/loda-ps.txt
  if ! grep loda /tmp/loda-ps.txt; then
  	start_miners $@
  fi
  rm /tmp/loda-ps.txt
}

trap abort_miners INT

push_updates
SECONDS=0
while [ true ]; do
  if (( SECONDS >= check_interval )); then
  	push_updates
    SECONDS=0
  fi
  sleep 600
done
