#!/bin/bash

# common settings
log_level=info
min_cpus=4

loda_cmd=
if [ -f "./loda" ] && [ -x "./loda" ]; then
  loda_cmd="./loda"
elif [ -x "$(command -v loda)" ]; then
  loda_cmd="loda"
else
  echo "Error: loda executable not found"
  exit 1
fi

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
if [ "$num_cpus" -ge 24 ]; then
  num_use_cpus=$(( $num_cpus - 8 ))
fi
if [ -n "$LODA_MAX_PROCESSES" ]; then
  num_use_cpus=$LODA_MAX_PROCESSES
fi

function log_info {
  echo "$(date +"%Y-%m-%d %H:%M:%S")|INFO |$@"
}

function ensure_miners() {

  # check how many loda processes are running
  ps -A > .loda-ps.txt
  num_proc=$(grep loda .loda-ps.txt | wc -l)
  rm .loda-ps.txt

  # number of processes to be started
  ((num_delta=num_use_cpus-num_proc))

  if [ "$num_delta" -gt 0 ]; then  
    i="$(date +"%-S")"
    ((i=i % 8))
    log_info "Starting ${num_delta}/${num_use_cpus} miner instances"
    for n in $(seq ${num_delta}); do
      ${loda_cmd} mine -l ${log_level} -i ${i} $@ &
      sleep 1
      ((i=i+1))
    done
  fi
}

function stop_miners() {
  log_info "Stopping loda"
  killall loda > /dev/null 2> /dev/null
  exit 1
}

trap stop_miners INT

while [ true ]; do
  ensure_miners $@
  sleep 21600
done
