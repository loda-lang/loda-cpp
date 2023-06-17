#!/bin/bash

export PATH=$PATH:/usr/local/bin:$HOME/loda/bin
export SLACK_CLI_TOKEN=...

for cmd in loda slack git rainbowstream; do
  if ! [ -x "$(command -v $cmd)" ]; then
    echo "Error: $cmd is not installed" >&2
    exit 1
  fi
done

echo "Stopping instances"
killall loda 2> /dev/null
sleep 3

echo "Cleaning up"
rm -r $HOME/loda/stats 2> /dev/null
rm $HOME/server-*.out 2> /dev/null
if [ -d $HOME/loda/programs/.git ]; then
  pushd $HOME/loda/programs > /dev/null
  git gc
  popd > /dev/null
fi

echo "Waiting for network"
while ! ping -c 1 loda-lang.org &> /dev/null 2>&1; do
  sleep 5
done

# number of instances
num_new=1
num_update=1

pushd $HOME > /dev/null

for i in $(seq 1 ${num_new}); do
  echo "Starting validate-new-${i}"
  nohup loda mine -i validate-new > $HOME/validate-new-${i}.out 2>&1 &
  sleep 2
done

for i in $(seq 1 ${num_update}); do
  echo "Starting validate-update-${i}"
  nohup loda mine -i validate-update > $HOME/validate-update-${i}.out 2>&1 &
  sleep 2
done

popd > /dev/null
