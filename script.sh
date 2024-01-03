#!/bin/bash 


if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <beginning port> <number of instances>"
fi

declare -i p=$1 n=$[$2-1]

./ServerLauncher $(($p)) &
sleep 1
for ((i = 0; i < n; i++)); do
   ./ServerLauncher $(($p + $i+1)) $(($p + $i)) &;
   sleep 1
  #./ServerLauncher
done

#pkill -9 -f "./ServerLauncher"