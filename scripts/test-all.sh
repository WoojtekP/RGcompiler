#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}/..
. ./scripts/config.sh

function runPerft() {
testparam=$1
g++ test/perft.cpp ${BUILD_TEST_DIR}/reasoner.cpp -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testperft
exitCode=$?
if [ "${exitCode}" -ne 0 ]; then
  elapsedTime=0
  result="Compilation error"
else
  startTime=$(date +%s.%3N)
  result=$(${BUILD_TEST_DIR}/testperft ${testparam})
  exitCode=$?
  endTime=$(date +%s.%3N)
  elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
  if [ "${exitCode}" -ne 0 ]; then
    result="[exitCode=${exitCode}] ${result}"
  fi
fi
}
function runSims() {
testparam=$1
g++ test/simulations.cpp ${BUILD_TEST_DIR}/reasoner.cpp -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testsims
exitCode=$?
if [ "${exitCode}" -ne 0 ]; then
  elapsedTime=0
  result="Compilation error"
else
  startTime=$(date +%s.%3N)
  result=$(${BUILD_TEST_DIR}/testsims ${testparam})
  exitCode=$?
  endTime=$(date +%s.%3N)
  elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
  if [ "${exitCode}" -ne 0 ]; then
    result="[exitCode=${exitCode}] ${result}"
  fi
fi
}
function verifySimsResult() {
TOLERANCE=0.5
length=${#simsData[@]}
result=(${result})
if [[ ${length} != ${#result[@]} ]]; then
return 1
fi
for (( i=0; i<${length}; ++i )); do
  if [[ `echo "((${simsData[i]}-${result[i]}))^2 > (${simsData[i]}*${TOLERANCE})^2" | bc 2>/dev/null` == 1 ]]; then
    return 1
  fi
done
return 0
}

WIDTH=70

tests=()
tests+=('ticTacToe.rg    1000000 5 7.63 9 64.84 35.16       1 9 72 504 3024 15120 54720') # 148176 200448 127872'
tests+=('ticTacToe.rbg   1000000 5 7.63 9 64.84 35.16       1 9 72 504 3024 15120 54720') # 148176 200448 127872'
tests+=('breakthrough.rg  100000 11 64.10 127 50.92 49.08   1 22 484 11132 256036 6182818') # 149264638
tests+=('breakthrough.hrg 100000 11 64.10 127 50.92 49.08   1 22 484 11132 256036 6182818') # 149264638
tests+=('breakthrough.rbg 100000 11 64.10 127 50.92 49.08   1 22 484 11132 256036 6182818') # 149264638
tests+=('hex2.rbg          10000 3 3.50 4 50.00 50.00       1 4 12 24 12 0')
tests+=('hex9.rbg          10000 37 107.51 121 52.30 47.70  1 81 6480') # 511920 39929760'
tests+=('connect4.hrg     100000 7 21.31 42 55.72 44.28     1 7 49 343 2401 16807') # 117649 823536 5673234'
tests+=('amazons-smart.hrg  1000 32 71.46 90 50.10 49.90    1 2176') # 4307152'
tests+=('amazons-naive.hrg  1000 32 71.46 90 50.10 49.90    1 2176') # 4307152'

for testEntry in "${tests[@]}"; do
  testEntry=(${testEntry})
  game=${testEntry[0]}
  sims=${testEntry[1]}
  simsData=(${testEntry[@]:2:5})
  perftData=(${testEntry[@]:7})

  startTime=$(date +%s.%3N)
  ./scripts/compile.sh ${game} > /dev/null
  endTime=$(date +%s.%3N)

  elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
  echo "${game} compilation time ${elapsedTime} s"

  runSims ${sims}
  infoHead="${game} sims=${sims}: "
  verifySimsResult
  if [[ $? != 0 ]]; then
    infoRes="${RED}ERROR${RESET} Expected ${CYAN}${simsData[@]}${RESET} but got ${CYAN}${result[@]}${RESET}"
  else
    infoRes="${GREEN}OK${RESET} Expected ${CYAN}${simsData[@]}${RESET}, got ${CYAN}${result[@]}${RESET}"
  fi
  info=`printf "%s runtime %9s" "${infoHead}${infoRes}" "${elapsedTime} s"`
  echo -e "${info}"

  for (( depth=0; depth<${#perftData[@]}; ++depth )); do
    expected=${perftData[depth]}

    runPerft ${depth}

    infoHead="${game} d=${depth}: "
    if [ "${result}" != "${expected}" ]; then
      infoRes="${RED}ERROR${RESET} Expected ${CYAN}${expected}${RESET} but got ${CYAN}${result}${RESET}"
    else
      infoRes="${GREEN}OK${RESET} ${CYAN}${result}${RESET}${CYAN}${RESET}"
    fi
    info=`printf "%-${WIDTH}s runtime %9s" "${infoHead}${infoRes}" "${elapsedTime} s"`
    echo -e "${info}"
  done
  echo
done
