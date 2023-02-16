#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}/..
. ./scripts/config.sh

function runTranslation() {
startTime=$(date +%s.%3N)
./scripts/compile.sh ${game} > /dev/null 2>&1
exitCode=$?
endTime=$(date +%s.%3N)
elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc | sed -r 's/^(-?)\./\10./')
}
function runGCC() {
startTime=$(date +%s.%3N)
g++ -c ${BUILD_TEST_DIR}/reasoner.cpp -o ${BUILD_TEST_DIR}/reasoner.o -I${BUILD_TEST_DIR} ${GCC_FLAGS} &&
g++ test/simulations.cpp ${BUILD_TEST_DIR}/reasoner.o -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testsimul &&
g++ test/perft.cpp ${BUILD_TEST_DIR}/reasoner.o -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testperft &&
exitCode=$?
endTime=$(date +%s.%3N)
elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc | sed -r 's/^(-?)\./\10./')
}
function runCompilation() {
./scripts/compile.sh ${game} > /dev/null &&
g++ -c ${BUILD_TEST_DIR}/reasoner.cpp -o ${BUILD_TEST_DIR}/reasoner.o -I${BUILD_TEST_DIR} ${GCC_FLAGS} &&
g++ test/simulations.cpp ${BUILD_TEST_DIR}/reasoner.o -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testsimul &&
g++ test/perft.cpp ${BUILD_TEST_DIR}/reasoner.o -DTEST=1 -I${BUILD_TEST_DIR} ${GCC_FLAGS} -o ${BUILD_TEST_DIR}/testperft &&
exitCode=$?
if [ "${exitCode}" -ne 0 ]; then
  result="compilation error"
  return 1
fi
return 0
}
function runSims() {
testparam=$1
startTime=$(date +%s.%3N)
result=$(${BUILD_TEST_DIR}/testsimul ${testparam})
exitCode=$?
endTime=$(date +%s.%3N)
elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
if [ "${exitCode}" -ne 0 ]; then
  result="(exitCode=${exitCode}) ${result}"
fi
}
function runPerft() {
testparam=$1
startTime=$(date +%s.%3N)
result=$(${BUILD_TEST_DIR}/testperft ${testparam})
exitCode=$?
endTime=$(date +%s.%3N)
elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
if [ "${exitCode}" -ne 0 ]; then
  result="(exitCode=${exitCode}) ${result}"
fi
}
function verifySimsResult() {
TOLERANCE=0.1
length=${#simulData[@]}
result=(${result})
if [[ ${length} != ${#result[@]} ]]; then
return 1
fi
for (( i=0; i<${length}; ++i )); do
  if [[ `echo "((${simulData[i]}-${result[i]}))^2 > (${simulData[i]}*${TOLERANCE})^2" | bc 2>/dev/null` == 1 ]]; then
    return 1
  fi
done
return 0
}

declare -A simulTests
declare -A perftTests
simulTests['ticTacToe.rg']='1000000 7.63 64.84 35.16'
perftTests['ticTacToe.rg']='1 9 72 504 3024 15120 54720' # 148176 200448 127872
simulTests['ticTacToe.rbg']=${simulTests['ticTacToe.rg']}
perftTests['ticTacToe.rbg']=${perftTests['ticTacToe.rg']}
simulTests['breakthrough.rg']='100000 64.10 50.92 49.08'
perftTests['breakthrough.rg']='1 22 484 11132 256036 6182818' # 149264638
simulTests['breakthrough.hrg']=${simulTests['breakthrough.rg']}
perftTests['breakthrough.hrg']=${perftTests['breakthrough.rg']}
simulTests['breakthrough.rbg']=${simulTests['breakthrough.rg']}
perftTests['breakthrough.rbg']=${perftTests['breakthrough.rg']}
simulTests['hex2.rbg']='10000 3.50 50.00 50.00'
perftTests['hex2.rbg']='1 4 12 24 12 0'
simulTests['hex9.rbg']='10000 107.51 52.30 47.70'
perftTests['hex9.rbg']='1 81 6480' # 511920 39929760
simulTests['connect4.hrg']='100000 21.31 55.72 44.28'
perftTests['connect4.hrg']='1 7 49 343 2401 16807' # 117649 823536 5673234
simulTests['amazons.hrg']='1000 71.46 50.10 49.90'
perftTests['amazons.hrg']='1 2176' # 4307152
simulTests['amazons-naive.hrg']=${simulTests['amazons.hrg']}
perftTests['amazons-naive.hrg']=${perftTests['amazons.hrg']}
simulTests['amazons-smart.hrg']=${simulTests['amazons.hrg']}
perftTests['amazons-smart.hrg']=${perftTests['amazons.hrg']}

allGames=()
if [ -z "$1" ]; then
allGames+=('ticTacToe.rg')
allGames+=('ticTacToe.rbg')
allGames+=('breakthrough.rg')
allGames+=('breakthrough.hrg')
allGames+=('breakthrough.rbg')
allGames+=('hex2.rbg')
allGames+=('hex9.rbg')
allGames+=('connect4.hrg')
allGames+=('amazons-smart.hrg')
allGames+=('amazons-naive.hrg')
else
allGames+=("$1")
fi

totalStartTime=$(date +%s.%3N)

sumRuntime=0
WIDTH=70
for game in "${allGames[@]}"; do
  simulData=(${simulTests[$game]})
  perftData=(${perftTests[$game]})
  simCount=${simulData[0]}
  simulData=(${simulData[@]:1})

#   startTime=$(date +%s.%3N)
#   runCompilation
#   endTime=$(date +%s.%3N)
#   elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc)
#   echo "${game} compilation time ${elapsedTime} s"

  runTranslation
  if [[ ${exitCode} != 0 ]]; then
    info=`printf "%-${WIDTH}s time %9s" "$game translation: ${RED}ERROR${RESET} (exitcode=${exitCode})" "${elapsedTime} s"`
    echo -e "${info}\n"
    ((errorCount=errorCount+1))
    continue
  fi
  info=`printf "%-${WIDTH}s time %9s" "$game translation: ${GREEN}OK${RESET}${CYAN}${RESET}${CYAN}${RESET}" "${elapsedTime} s"`
  echo -e "${info}"

  runGCC
  if [[ ${exitCode} != 0 ]]; then
    info=`printf "%-${WIDTH}s time %9s" "$game g++: ${RED}ERROR${RESET} (exitcode=${exitCode})" "${elapsedTime} s"`
    echo -e "${info}\n"
    ((errorCount=errorCount+1))
    continue
  fi
  info=`printf "%-${WIDTH}s time %9s" "$game g++: ${GREEN}OK${RESET}${CYAN}${RESET}${CYAN}${RESET}" "${elapsedTime} s"`
  echo -e "${info}"

  runSims ${simCount}
  infoHead="${game} sims=${simCount}: "
  verifySimsResult
  if [[ $? != 0 ]]; then
    infoRes="${RED}ERROR${RESET} Expected ${CYAN}${simulData[@]}${RESET} but got ${CYAN}${result[@]}${RESET}"
  else
    infoRes="${GREEN}OK${RESET} Expected ${CYAN}${simulData[@]}${RESET}, got ${CYAN}${result[@]}${RESET}"
  fi
  info=`printf "%s   time %s" "${infoHead}${infoRes}" "${elapsedTime} s"`
  echo -e "${info}"
  sumRuntime=`echo "$sumRuntime + $elapsedTime" | bc`

  for (( depth=0; depth<${#perftData[@]}; ++depth )); do
    expected=${perftData[depth]}
    runPerft ${depth}
    infoHead="${game} depth=${depth}: "
    if [ "${result}" != "${expected}" ]; then
      infoRes="${RED}ERROR${RESET} Expected ${CYAN}${expected}${RESET} but got ${CYAN}${result}${RESET}"
      info=`printf "%s   time %s" "${infoHead}${infoRes}" "${elapsedTime} s"`
    else
      infoRes="${GREEN}OK${RESET} ${CYAN}${result}${RESET}${CYAN}${RESET}"
      info=`printf "%-${WIDTH}s time %9s" "${infoHead}${infoRes}" "${elapsedTime} s"`
    fi
    echo -e "${info}"
    sumRuntime=`echo "$sumRuntime + $elapsedTime" | bc`
  done

  echo
done

totalEndTime=$(date +%s.%3N)
elapsedTime=$(echo "scale=3; ${totalEndTime} - ${totalStartTime}" | bc)
echo "Finished in ${elapsedTime} s   Total runtime ${sumRuntime} s"
