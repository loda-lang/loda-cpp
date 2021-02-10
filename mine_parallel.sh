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
check_interval=43200
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

function start_miners() {

  # configuration
  local l="-l ${log_level}"

  num_inst=$(( $num_cpus / 2 ))
  if [ "$num_inst" -ge 8 ]; then
    num_inst=$(( $num_inst - 2 ))
  fi

  # start miners
  echo "Start mining using ${num_cpus} instances"
  for n in $(seq ${num_inst}); do
    ./loda mine $l $@ &
    ./loda mine $l -x $@ &
  done

  # maintenance
  if [ "$branch" = "master" ]; then
    echo "Start maintenance"
    ./loda maintain &
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
    echo "Pushing updates"
    git add programs
    if [ "$branch" = "master" ]; then
      git add stats
    fi
    num_progs=$(cat stats/summary.csv | cut -d , -f 1)
    git commit -m "updated ${num_changes}/${num_progs} programs"
    git pull -r
    if [ "$branch" != "master" ]; then
      git merge -X theirs -m "merge master into $branch" origin/master
    fi
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
