#!/usr/bin/env bash

out1=$1
out2=$2
for ((i = 1; i <= 50; i++)); do
  cnt1=$(cat $out1 | rg -e ".* $i$" | wc -l)
  cnt2=$(cat $out2 | rg -e ".* $i$" | wc -l)
  if [[ $cnt1 != $cnt2 ]]; then
    echo "check not passed"
    exit -1
  fi
done
echo "check passed"