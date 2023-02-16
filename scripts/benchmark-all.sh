#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}/..
. ./scripts/config.sh

INSTR_SCALE=1000000

perf_event_paranoid=`cat /proc/sys/kernel/perf_event_paranoid`
if (( $perf_event_paranoid > 3 )); then
echo "perf_event_paranoid is ${perf_event_paranoid} but must be <= 3"
echo "Fix with: sudo echo 3 > /proc/sys/kernel/perf_event_paranoid"
exit 1
fi

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
function runSimBenchmark() {
testparam=$1
startTime=$(date +%s.%3N)
elapsedInstr=`perf stat -e instructions -x " " ${BUILD_TEST_DIR}/testsimul ${testparam} 2>&1 > /dev/null`
endTime=$(date +%s.%3N)
if [[ "${exitCode}" == 0 ]]; then
  elapsedInstr=(${elapsedInstr[0]})
  elapsedInstr=`echo "${elapsedInstr} / ${INSTR_SCALE}" | bc`
fi
elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc | sed -r 's/^(-?)\./\10./')
}
function runPerftBenchmark() {
testparam=$1
startTime=$(date +%s.%3N)
elapsedInstr=`perf stat -e instructions -x " " ${BUILD_TEST_DIR}/testperft ${testparam} 2>&1 > /dev/null`
exitCode=$?
endTime=$(date +%s.%3N)
if [[ "${exitCode}" == 0 ]]; then
  elapsedInstr=(${elapsedInstr[0]})
  elapsedInstr=`echo "${elapsedInstr} / ${INSTR_SCALE}" | bc`
fi
elapsedTime=$(echo "scale=3; $endTime - $startTime" | bc | sed -r 's/^(-?)\./\10./')
}

tests=()
if [[ $# -eq 0 ]]; then
tests+=('breakthrough.rg 100000 5')
tests+=('breakthrough.hrg 100000 5')
tests+=('breakthrough.rbg 100000 5')
tests+=('connect4.hrg 100000 5')
tests+=('amazons-smart.hrg 1000 1')
elif [[ $# -eq 3 ]]; then
tests+=("$1","$2","$3")
else
echo "To run all predefined benchmark tests:"
echo "./test-all.sh"
echo "To run one custom benchmark test:"
echo "./test-all.sh [game] [simulations number] [perft depth]"
fi

sumTranslationTime=0
sumGCCTime=0
sumSimTime=0
sumSimInstr=0
sumPerftTime=0
sumPerftInstr=0
errorCount=0
WIDTH=40
for testEntry in "${tests[@]}"; do
  testEntry=(${testEntry})
  game=${testEntry[0]}
  simCount=${testEntry[1]}
  perftDepth=${testEntry[2]}

  runTranslation
  if [[ ${exitCode} != 0 ]]; then
    info=`printf "%-${WIDTH}s %9s" "$game translation" "${RED}ERROR${RESET} (exitcode=${exitCode})"`
    echo -e "${info}\n"
    ((errorCount=errorCount+1))
    continue
  fi
  info=`printf "%-${WIDTH}s time %9s" "$game translation" "${elapsedTime} s"`
  echo -e "${info}"
  sumTranslationTime=`echo "$sumTranslationTime + $elapsedTime" | bc`

  runGCC
  if [[ ${exitCode} != 0 ]]; then
    info=`printf "%-${WIDTH}s %9s" "$game g++" "${RED}ERROR${RESET} (exitcode=${exitCode})"`
    echo -e "${info}\n"
    ((errorCount=errorCount+1))
    continue
  fi
  info=`printf "%-${WIDTH}s time %9s" "$game g++" "${elapsedTime} s"`
  echo -e "${info}"
  sumGCCTime=`echo "$sumGCCTime + $elapsedTime" | bc`

  runSimBenchmark ${simCount}
  if [[ ${exitCode} != 0 ]]; then
    info=`printf "%-${WIDTH}s %9s" "$game sims=${simCount}" "${RED}ERROR${RESET} (exitcode=${exitCode})"`
    ((errorCount=errorCount+1))
  else
    info=`printf "%-${WIDTH}s time %9s   instr %7s" "${game} sims=${simCount}" "${elapsedTime} s" "${elapsedInstr}"`
    sumSimTime=`echo "$sumSimTime + $elapsedTime" | bc`
    sumSimInstr=`echo "$sumSimInstr + $elapsedInstr" | bc`
  fi
  echo -e "${info}"

  runPerftBenchmark ${perftDepth}
  if [[ ${exitCode} != 0 ]]; then
    info=`printf "%-${WIDTH}s %9s" "$game depth=${perftDepth}" "${RED}ERROR${RESET} (exitcode=${exitCode})"`
    ((errorCount=errorCount+1))
  else
    info=`printf "%-${WIDTH}s time %9s   instr %7s" "${game} perft depth=${perftDepth}" "${elapsedTime} s" "${elapsedInstr}"`
    sumPerftTime=`echo "$sumPerftTime + $elapsedTime" | bc`
    sumPerftInstr=`echo "$sumPerftInstr + $elapsedInstr" | bc`
  fi
  echo -e "${info}"

  echo
done

echo "---"
if [[ ${errorCount} != 0 ]]; then
info="Benchmark ${RED}ERRORS${RESET}: ${errorCount}"
echo -e "${info}"
else
info="Benchmark ${GREEN}OK${RESET}"
fi
info=`printf "%-50s %9s" "Total translation time" "${sumTranslationTime} s"`
echo -e "${info}"
info=`printf "%-50s %9s" "Total g++ time" "${sumGCCTime} s"`
echo -e "${info}"
info=`printf "%-50s %9s %7s %9s" "Total sim time" "${sumSimTime} s" "instr" "${sumSimInstr}"`
echo -e "${info}"
info=`printf "%-50s %9s %7s %9s" "Total perft time" "${sumPerftTime} s" "instr" "${sumPerftInstr}"`
echo -e "${info}"
