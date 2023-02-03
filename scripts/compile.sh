#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}/..
. ./scripts/config.sh

if [[ $# -ne 1 ]] ; then
  echo "usage: ./compile.sh [game]"
  echo "example: ./compile.sh breakthrough.rg"
  exit 1
fi

set -e
set -o pipefail

game=$1
echo "Compiling $1"

mkdir -p ${BUILD_TEST_DIR} > /dev/null
cp defaultMap.hpp ${BUILD_TEST_DIR}/defaultMap.hpp

# create AST
start_time=$(date +%s.%3N)
node ${RG_DIR}/interpreter_node/lib/cli rg-source ${RG_DIR}/examples/${game} > ${BUILD_TEST_DIR}/game-tmp.rg
node ${RG_DIR}/interpreter_node/lib/cli --expandGeneratorNodes --compactSkipEdges rg-ast ${BUILD_TEST_DIR}/game-tmp.rg > ${BUILD_TEST_DIR}/${game}.json
cd ${BUILD_TEST_DIR}
python3 -m json.tool ${game}.json > ${game}-ast.json
rm game-tmp.rg
end_time=$(date +%s.%3N)
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
printf "%-20s" "ast:"
printf "%10s\n" "${elapsed} s"

# generate cpp files
start_time=$(date +%s.%3N)
../build/rg2cpp --file ${game}-ast.json --opt-conditions 6
end_time=$(date +%s.%3N)
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
printf "%-20s" "rg2cpp:"
printf "%10s\n" "${elapsed} s"

# format generated files
start_time=$(date +%s.%3N)
clang-format -style=file -i reasoner.hpp reasoner.cpp
end_time=$(date +%s.%3N)
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
printf "%-20s" "clang-format:"
printf "%10s\n" "${elapsed} s"
