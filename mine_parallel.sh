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
check_interval=21600
min_cpus=4
branch=$(git rev-parse --abbrev-ref HEAD)
remote_origin=$(git config --get remote.origin.url)

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

# determine how many cpus to use
num_use_cpus=$num_cpus
if [ "$num_cpus" -ge 16 ]; then
  num_use_cpus=$(( $num_cpus - 4 ))
fi

# calculate minimum number of changes before commit
min_changes=$(( $num_cpus * 2 ))
min_changes=1000

# increase metric publishing interval because we run multiple instances in parallel
export LODA_METRICS_PUBLISH_INTERVAL=600

function stop_miners() {
  echo "Stopping miners"
  killall loda > /dev/null
}

function abort_miners() {
  stop_miners
  exit 1
}

function start_miners() {

  # maintenance
  if [ "$remote_origin" = "git@github.com:ckrause/loda.git" ] && [ "$branch" = "master" ]; then
    echo "Start maintenance"
    ./loda maintain &
    sleep 90
  fi

  # start miners
  i=0
  echo "Start mining using ${num_use_cpus} instances"
  for n in $(seq ${num_use_cpus}); do
    ./loda mine -l ${log_level} -i ${i} $@ &
    ((i=i+1))
  done
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
    git add programs/oeis
    num_progs=$(cat $HOME/.loda/stats/summary.csv | tail -n 1 | cut -d , -f 1)
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
