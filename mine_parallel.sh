#!/bin/bash

log_level=info
restart_interval=86400
min_changes=20

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
  echo "Starting loda $@"
  ./loda $@ &
}

function start_miners() {
  echo "Updating OEIS database"
  ./get_oeis.sh
  echo "Start mining"
  local l="-l ${log_level}"
  for n in 8 10; do
    for x in "" "-x"; do
      args="-n $n -p ${n}0 -a cd $x $l"
      # instantiate templates w/o loops
      for t in T01 T02; do
        run_loda mine $args -o ^l -e programs/templates/${t}.asm $@
      done
      # no templates but w/ loops
      run_loda mine $args $@
    done
  done
  # indirect memory access
  run_loda mine -p 60 -a cdi -n 6 $l $@
  run_loda maintain
}

function push_updates {
  if [ ! -d "programs" ]; then
    echo "programs folder not found; aborting"
    exit 1
  fi
  num_changes=$(git status programs -s | wc -l)
  if [ "$num_changes" -ge "$min_changes" ]; then
    echo "Pushing updates"
    git pull
    git add programs lengths.png README.md
    git commit -m "updated $num_changes programs"
    git push
    echo "Rebuilding loda"
    pushd src && make clean && make && popd
  fi
}

trap abort_miners INT

push_updates
start_miners $@
SECONDS=0
while [ true ]; do
  if (( SECONDS >= restart_interval )); then
  	stop_miners
    ./make_charts.sh 
  	push_updates
  	start_miners $@
    SECONDS=0
  fi
  sleep 60
done
