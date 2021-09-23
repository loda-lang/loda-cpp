#!/bin/bash

# common settings
log_level=info
min_cpus=4

loda_cmd=
if [ -x "./loda" ]; then
  loda_cmd="./loda"
elif [ -x "$(command -v $cmd)" ]; then
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

function start_miners() {
  # i=0
  i=1
  log_info "Starting ${num_use_cpus} miner instances"
  for n in $(seq ${num_use_cpus}); do
    ${loda_cmd} mine -l ${log_level} -i ${i} $@ &
    sleep 1
    ((i=i+1))
  done
  ${loda_cmd} mine -l ${log_level} -i blocks -c 100000 $@ &
}

function stop_miners() {
  log_info "Stopping loda"
  killall loda > /dev/null 2> /dev/null
  exit 1
}

trap stop_miners INT

start_miners $@
while [ true ]; do
  sleep 600
done
