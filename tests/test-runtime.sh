#!/bin/bash

# set the Slack token here!
export SLACK_CLI_TOKEN=
export PATH=$PATH:/usr/local/bin:$HOME/loda/bin

if [ -z "$SLACK_CLI_TOKEN" ]; then
  echo "Error: SLACK_CLI_TOKEN is not set"
  exit 1
fi

for cmd in slack git; do
  if ! [ -x "$(command -v $cmd)" ]; then
    echo "Error: $cmd is not installed" >&2
    exit 1
  fi
done

pushd .. > /dev/null

# rebuild
git pull
COMMIT=$(git rev-parse HEAD)
pushd src > /dev/null
make clean ; make
popd > /dev/null

# start miner (2 hours)
PROFILE=mutate
./loda mine -i $PROFILE -H 2 &
PID=$!

START_TIME=$SECONDS
WARN_TIME=10800
ERR_TIME=14400

while true; do
  # check if the process is still running
  if ! ps -p $PID > /dev/null; then
    break
  fi
  # check runtime
  DURATION=$((SECONDS - START_TIME))
  if (( DURATION > ERR_TIME )); then
    kill $PID
    break
  fi
  sleep 10
done

DURATION=$((SECONDS - START_TIME))
if (( DURATION > ERR_TIME )); then
  TEXT="Task exceeded time limit"
  COLOR="danger"
elif (( DURATION > WARN_TIME )); then
  TEXT="Task ran unusually long"
  COLOR="warning"
else
  TEXT="Task finished on time"
  COLOR="good"
fi
slack chat send --text "${TEXT}: ${DURATION}s; profile: $PROFILE; commit: $COMMIT" --color "$COLOR" --channel "#test-runtime"

popd > /dev/null
