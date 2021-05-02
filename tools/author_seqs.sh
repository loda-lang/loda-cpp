#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <author>"
  exit 1
fi
author=$1

for cmd in cat git grep wget; do
  if ! [ -x "$(command -v $cmd)" ]; then
    echo "Error: $cmd is not installed" >&2
    exit 1
  fi
done

lodaroot=$(git rev-parse --show-toplevel)

asinfo=$HOME/.loda/oeis/asinfo.txt
if [ ! -f $asinfo ]; then
  mkdir -p $HOME/.loda/oeis
  wget -nv --no-use-server-timestamps -O $asinfo http://www.teherba.org/OEIS-mat/common/asinfo.txt
fi

seqs=$(cat $asinfo | grep "_${author}_" | cut -f1)

for s in $seqs; do
  p=${lodaroot}/programs/oeis/${s:1:3}/${s}.asm
  if [ -f $p ]; then
    head -n 1 $p
  fi
done
