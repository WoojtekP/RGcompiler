#!/bin/bash

if [[ $# -ne 2 ]] ; then
  echo "usage: simulations.sh [game] [numberOfSimulations]"
  exit 1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}

game=$1
numberOfSimulations=$2

./compile.sh $game
cd ../test
make
echo "Running simulations..."
./simulations $numberOfSimulations
