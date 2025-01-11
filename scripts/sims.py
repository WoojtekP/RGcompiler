#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run simulations.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('sims', nargs=1, help='number of simulations (int) or time in seconds (float, ended with "s")')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-skipcompilation', action='store_true', help='skip compile.py and use the existing reasoner sources')
parser.add_argument('-perf', action='store_true', help='count instructions by perf')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('-debug', action='store_true', help='compile with gdb symbols')
cpp_flags_group.add_argument('-profile', action='store_true', help='generate profiler information')
cpp_flags_group.add_argument('-benchmark', action='store_true', help='maximum speed flags')

args = parser.parse_args()
game = args.game[0]
if args.sims[0].endswith('s'):
  useTime = 1
  sims = int(float(args.sims[0][:-1]) * 1000)
else:
  useTime = 0
  sims = int(args.sims[0])
translateOptions = '"' + args.translateOptions + '"'

if not args.skipcompilation:
  print(f'Translate options: {translateOptions}')
  run(f'python3 scripts/compile.py {game} -t{translateOptions}')

HEAD_FORMATTER = '{: <14} '
TIME_FORMATTER = '{:9.3f} s'
FULL_FORMATTER = HEAD_FORMATTER + TIME_FORMATTER

INSTR_SCALE = 1_000_000
STATESSTAT_FORMATTER = '  {:15,.0f} states/s'
SIMSSTAT_FORMATTER = '  {:15,.0f} sims/s'
INSTR_FORMATTER = ' {:9,.0f} mil instr'

usePerf = args.perf

if args.profile:
  gccFlags = cfg.GCC_PROFILE_FLAGS
  infoGccFlags = "profile"
elif args.debug:
  gccFlags = cfg.GCC_DEBUG_FLAGS
  infoGccFlags = "debug"
elif args.benchmark:
  gccFlags = cfg.GCC_BENCHMARK_FLAGS
  infoGccFlags = "benchmark"
else:
  gccFlags = cfg.GCC_TEST_FLAGS
  infoGccFlags = "test"

startTime = time.time()
run(f'g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {gccFlags} -DUSE_TIME={useTime} -o {cfg.BUILD_TEST_DIR}/sims')
elapsedTime = time.time() - startTime
print(f'g++ {infoGccFlags} flags: {gccFlags}')
print(FULL_FORMATTER.format('g++:',elapsedTime))

print(HEAD_FORMATTER.format(f'sims {str(sims)}:'),end='',flush=True)
startTime = time.time()
if usePerf:
  result = runCap(f'perf stat -e instructions -x " " {cfg.BUILD_TEST_DIR}/sims {sims}')
else:
  result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {sims}')
elapsedTime = time.time() - startTime
if result.returncode != 0:
  print(f'{util.ERROR} exitcode {result.returncode}')
  print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
else:
  statesCount = int(decodeOutput(result.stdout).strip().split(' ')[0])
  if usePerf:
    output = decodeOutput(result.stderr)
    if str.isnumeric(output.split(' ')[0]):
      elapsedInstr = int(output.split(' ')[0]) / INSTR_SCALE
      print((TIME_FORMATTER+INSTR_FORMATTER+STATESSTAT_FORMATTER+SIMSSTAT_FORMATTER).format(elapsedTime, elapsedInstr, statesCount/elapsedTime, sims/elapsedTime).replace(',',' '))
    else:
      print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
      print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
  else:
    print((TIME_FORMATTER+STATESSTAT_FORMATTER+SIMSSTAT_FORMATTER).format(elapsedTime, statesCount/elapsedTime, sims/elapsedTime))

if result.returncode != 0:
  print(f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}) {decodeOutput(result.stderr)}{util.RESET}')
  exit(2)
stats = decodeOutput(result.stdout).strip().split(' ')
resStates = int(stats[0])
resAvgDepth = resStates / sims
resMinDepth = int(stats[1])
resMaxDepth = int(stats[2])
resMoves = int(stats[3])
resMinMoves = int(stats[4])
resMaxMoves = int(stats[5])
stats = stats[6:]
resAvgScores = []
for p in range(len(stats)): resAvgScores.append(int(stats[p]) / sims)
print(f'states: {resStates}')
print(f'depth: min {resMinDepth} avg {resAvgDepth:1.2f} max {resMaxDepth}')
print(f'scores: avg {" ".join(f"{avgScore:1.2f}" for avgScore in resAvgScores)}')

if args.profile:
  run(f'gprof {cfg.BUILD_TEST_DIR}/sims gmon.out > gmon.txt')
