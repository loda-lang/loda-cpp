#!/bin/bash

log_level=info

trap abort INT

function abort() {
  echo "Exiting..."
  killall loda
  exit 1
}

for n in 2 4 6; do
  p="${n}0"
  l="-l ${log_level}"
  ./loda mine -p $p -a cd -o asm -e programs/templates/T01.asm $l &
  ./loda mine -p $p -a cd -o asm -e programs/templates/T02.asm $l &
  ./loda mine -p $p -a cd -n $n $l &
  ./loda mine -p $p -n $n $l &
done

input=
while [ "$input" != "exit" ]; do
  read input
done
