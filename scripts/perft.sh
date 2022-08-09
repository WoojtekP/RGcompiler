#!/bin/bash

if [[ $# -ne 2 ]] ; then
  echo "usage: perft.sh [game] [depth]"
  exit 1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}

game=$1
depth=$2

./compile.sh $game
cd ../test
make perft
echo "Running perft depth=${depth} ..."
./perft $depth
