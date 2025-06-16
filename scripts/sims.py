#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run simulations.')
parser.add_argument('game', nargs=1, help='game name')
parser.add_argument('limit', nargs=1, help='number of simulations (int) or time in seconds (float, ended with "s")')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-skipcompilation', action='store_true', help='skip compile.py and use the existing reasoner sources')
parser.add_argument('-perf', action='store_true', help='count instructions by perf')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('-benchmark', action='store_true', help='maximum speed flags')
cpp_flags_group.add_argument('-test', action='store_true', help='optimization with asserts (default)')
cpp_flags_group.add_argument('-debug', action='store_true', help='compile with gdb symbols')
cpp_flags_group.add_argument('-profile', action='store_true', help='generate profiler information')

args = parser.parse_args()

game = args.game[0]
if args.limit[0].endswith('s'):
  useTime = 1
  limitS = float(args.limit[0][:-1])
  limit = int(round(limitS * 1000.0))
else:
  useTime = 0
  limit = int(args.limit[0])

translateOptions = args.translateOptions

HEAD_FORMATTER = '{: <50} '
TIME_FORMATTER = '{:7.3f}s'
FULL_FORMATTER = HEAD_FORMATTER + TIME_FORMATTER

INSTR_SCALE = 1_000
STATES_STAT_FORMATTER = ' {:15,.0f} states/s'
STATES_STAT_PRECISE_FORMATTER = ' {:11,.3f} states/s'
SIMS_STAT_FORMATTER = ' {:15,.0f} sims/s'
SIMS_STAT_PRECISE_FORMATTER = ' {:11,.3f} sims/s'
INSTR_FORMATTER = ' {:12,.0f} k instr'

usePerf = args.perf

if args.profile:
  gccOptions = cfg.GCC_PROFILE_FLAGS
  infoGccOptions = "profile"
elif args.debug:
  gccOptions = cfg.GCC_DEBUG_FLAGS
  infoGccOptions = "debug"
elif args.benchmark:
  gccOptions = cfg.GCC_BENCHMARK_FLAGS
  infoGccOptions = "benchmark"
else:
  gccOptions = cfg.GCC_TEST_FLAGS
  infoGccOptions = "test"

print(f'Testing: {game}')
print(f'Translate options: {translateOptions}')
print(f'rg2cpp options: {cfg.DEFAULT_RG2CPP_OPTIONS}')
print(f'g++ {infoGccOptions} options: {gccOptions}')
if usePerf: print(f'Using perf')
print()
    
#######################################################################################################################

### Compile ###
if not args.skipcompilation:
  print(HEAD_FORMATTER.format(f'{game} compile:'),end='',flush=True)
  startTime = time.time()
  run(f'python3 scripts/compile.py {game} -t"{translateOptions}" -silent')
  elapsedTime = time.time() - startTime
  print(TIME_FORMATTER.format(elapsedTime))

### g++ ###
print(HEAD_FORMATTER.format(f'{game} g++:'),end='',flush=True)
startTime = time.time()
run(f'g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {gccOptions} -DUSE_TIME={useTime} -o {cfg.BUILD_TEST_DIR}/sims')
elapsedTime = time.time() - startTime
print(TIME_FORMATTER.format(elapsedTime))

### Sims ###
if useTime:
  print(HEAD_FORMATTER.format(f'{game} sims time {limitS}s:'),end='',flush=True)
else:
  print(HEAD_FORMATTER.format(f'{game} sims count {limit}:'),end='',flush=True)
startTime = time.time()
if usePerf:
  result = runCap(f'perf stat -e instructions -x " " {cfg.BUILD_TEST_DIR}/sims {limit}')
else:
  result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {limit}')
elapsedTime = time.time() - startTime
if result.returncode != 0:
  print(f'{util.ERROR} exitcode {result.returncode}')
  print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
  print(f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}) {decodeOutput(result.stderr)}{util.RESET}')
  exit(2)

stats = decodeOutput(result.stdout).strip().split(' ')
resSims = int(stats[1])
resStates = int(stats[2])
if usePerf:
  output = decodeOutput(result.stderr)
  if str.isnumeric(output.split(' ')[0]):
    elapsedInstr = int(output.split(' ')[0]) / INSTR_SCALE
    print((TIME_FORMATTER+INSTR_FORMATTER+STATES_STAT_FORMATTER+SIMS_STAT_FORMATTER).format(elapsedTime, elapsedInstr, resStates/elapsedTime, resSims/elapsedTime).replace(',',' '))
  else:
    print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
    exit(2)
else:
  print(TIME_FORMATTER.format(elapsedTime),end='')
  formatter = STATES_STAT_PRECISE_FORMATTER if resStates < 10 else STATES_STAT_FORMATTER
  print(formatter.format(resStates/elapsedTime).replace(',',' '),end='')
  formatter = SIMS_STAT_PRECISE_FORMATTER if resSims < 10 else SIMS_STAT_FORMATTER
  print(formatter.format(resSims/elapsedTime).replace(',',' '))

#######################################################################################################################
print()
resAvgDepth = resStates / resSims
resMinDepth = int(stats[2])
resMaxDepth = int(stats[3])
resMoves = int(stats[4])
resMinMoves = int(stats[5])
resMaxMoves = int(stats[6])
stats = stats[7:]
resAvgScores = []
resMinScores = []
resMaxScores = []
for p in range(len(stats)//3):
  resAvgScores.append(int(stats[p*3]) / resSims)
  resMinScores.append(int(stats[p*3+1]))
  resMaxScores.append(int(stats[p*3+2]))
print(f'sims: {resSims}')
print(f'states: {resStates}')
print(f'depth: min {resMinDepth} avg {resAvgDepth:1.2f} max {resMaxDepth}')
print(f'moves: min {resMinMoves} avg {resMoves/resStates:1.2f} max {resMaxMoves}')
print(f'avg scores: {" ".join(f"{avgScore:1.2f}" for avgScore in resAvgScores)}')
print(f'min scores: {" ".join(f"{minScore}" for minScore in resMinScores)}')
print(f'max scores: {" ".join(f"{maxScore}" for maxScore in resMaxScores)}')

if args.profile:
  run(f'gprof {cfg.BUILD_TEST_DIR}/sims gmon.out > gmon.txt')
