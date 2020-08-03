#!/bin/bash

# common settings
log_level=error
check_interval=172800
min_changes=20
min_cpus=4

# get and check the number of cpus
if [[ "$OSTYPE" == "darwin"* ]]; then
  num_cpus=$(sysctl -n hw.ncpu)
else
  num_cpus=$(cat /proc/cpuinfo | grep processor | wc -l)
fi
if [ "$num_cpus" -lt "$min_cpus" ]; then
  echo "Need at least ${min_cpus} CPUs, but only ${num_cpus} were found"
fi

# check required commands
for cmd in git; do
  if ! [ -x "$(command -v $cmd)" ]; then
    echo "Error: $cmd is not installed" >&2
    exit 1
  fi
done

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
  echo "Start mining"
  local l="-l ${log_level}"
  templates=T01
  num_vars=4
  if [ "$num_cpus" -ge 8 ]; then
    templates="T01 T02"
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
  if [ "$num_cpus" -ge 28 ]; then
    num_vars="2 4 6 7 8 10"
  fi
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
  run_loda mine -n 6 -p 60 -a cdi $l $@
  run_loda maintain
}

function push_updates {
  if [ ! -d "programs" ]; then
    echo "programs folder not found; aborting"
    exit 1
  fi
  num_changes=$(git status programs -s | wc -l)
  if [ "$num_changes" -ge "$min_changes" ]; then
  	stop_miners
    ./make_charts.sh 
    echo "Pushing updates"
    git pull
    git add programs
    branch=$(git branch --show-current)
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
