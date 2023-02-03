#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}/..
. ./scripts/config.sh

if [[ $# -ne 2 ]] ; then
  echo "usage: simulations.sh [game] [count]"
  exit 1
fi

game=$1
depth=$2

./scripts/compile.sh ${game}

start_time=$(date +%s.%3N)
g++ test/perft.cpp ${BUILD_TEST_DIR}/reasoner.cpp -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/perft
end_time=$(date +%s.%3N)
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
printf "%-20s" "g++ perft:"
printf "%10s\n" "${elapsed} s"

echo "Running perft depth=${depth} ..."
${BUILD_TEST_DIR}/perft ${depth}
