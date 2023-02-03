#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}/..
. ./scripts/config.sh

function runTestVerbose() {
testparam=$1

start_time=$(date +%s.%3N)
g++ test/perft.cpp ${BUILD_TEST_DIR}/reasoner.cpp -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testperft
end_time=$(date +%s.%3N)
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
printf "%-20s" "g++ perft(${testparam}):"
printf "%10s\n" "${elapsed} s"

start_time=$(date +%s.%3N)
result=`${BUILD_TEST_DIR}/testperft ${testparam}`
end_time=$(date +%s.%3N)
elapsed=$(echo "scale=3; $end_time - $start_time" | bc)
printf "%-20s" "perft(${testparam}):"
printf "%10s\n" "${elapsed} s"
}
function runTest() {
testparam=$1
g++ test/perft.cpp ${BUILD_TEST_DIR}/reasoner.cpp -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testperft
startTime=$(date +%s.%3N)
result=`${BUILD_TEST_DIR}/testperft ${testparam}`
endTime=$(date +%s.%3N)
elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
}

tests[0]='ticTacToe.rg 1 9 72 504 3024 15120 54720' # 148176 200448 127872'
tests[1]='ticTacToe.rbg 1 9 72 504 3024 15120 54720' # 148176 200448 127872'
tests[2]='breakthrough.rg 1 22 484 11132 256036 6182818 149264638'
tests[3]='breakthrough.hrg 1 22 484 11132 256036 6182818 149264638'
tests[4]='breakthrough.rbg 1 22 484 11132 256036 6182818 149264638'
tests[5]='hex2.rbg 1 4 12 24 12 0'
tests[6]='hex9.rbg 1 81 6480' # 511920 39929760'
tests[7]='connect4.hrg 1 7 49 343 2401 16807' # 117649 823536 5673234'
tests[8]='amazons-smart.hrg 1 2176' # 4307152'
tests[9]='amazons-naive.hrg 1 2176' # 4307152'

for testEntry in "${tests[@]}"; do
  game=( ${testEntry}[0] )
  expectedResults=(${testEntry})
  expectedResults=(${expectedResults[@]:1})

  startTime=$(date +%s.%3N)
  # TODO failed compilation
  ./scripts/compile.sh ${game} > /dev/null
  endTime=$(date +%s.%3N)
  elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
  echo "${game} compilation time ${elapsedTime} s"

  for (( depth=0; depth<${#expectedResults[@]}; ++depth )); do
    expected=${expectedResults[depth]}

    runTest ${depth}

    infoHead="${game} d=${depth}: "
    if [ "${result}" != "${expected}" ]; then
      infoRes="${RED}ERROR${RESET} Expected ${expected} but got \"${result}\""
    else
      infoRes="${GREEN}OK${RESET} ${result}"
    fi
    info=`printf "%-100s runtime %9s" "${infoHead}${infoRes}" "${elapsedTime} s"`
    echo -e "${info}"
  done
done
