#!/bin/bash

log_level=info

trap abort INT

function abort() {
  echo "Exiting..."
  killall loda
  exit 1
}

for n in 4 6 8; do
  ./loda mine -n ${n} -p ${n}0 -a cd -l ${log_level} &
done

for p in 20 40; do
  ./loda mine -p ${p} -a cd -o asm -e programs/templates/T01.asm -l ${log_level} &
done

for p in 20 40 60; do
  ./loda mine -p ${p} -a cd -o asm -e programs/templates/T02.asm -l ${log_level} &
done

for p in 40 60; do
  ./loda mine -p ${p} -l ${log_level} &
done

input=
while [ "$input" != "exit" ]; do
  read input
done
