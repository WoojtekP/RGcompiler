#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run perft.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('depth', nargs=1, help='perft depth')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-skipcompilation', action='store_true', help='skip compile.py and use the existing reasoner sources')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('-debug', action='store_true', help='compile with gdb symbols')
cpp_flags_group.add_argument('-profile', action='store_true', help='generate profiler information')
cpp_flags_group.add_argument('-benchmark', action='store_true', help='maximum speed flags')

args = parser.parse_args()
game = args.game[0]
depth = args.depth[0]
translateOptions = args.translateOptions
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
#if usePerf: print(f'Using perf')

#######################################################################################################################

if not args.skipcompilation:
  run(f'python3 scripts/compile.py {game} -t"{translateOptions}" -silent')

HEAD_FORMATTER = '{: <14} '
TIME_FORMATTER = '{:9.3f} s'
FULL_FORMATTER = HEAD_FORMATTER + TIME_FORMATTER

startTime = time.time()
run(f'g++ test/perft.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {cfg.GCC_TEST_FLAGS} -o {cfg.BUILD_TEST_DIR}/perft')
elapsedTime = time.time() - startTime
print(FULL_FORMATTER.format('g++:',elapsedTime))

print(HEAD_FORMATTER.format(f'perft {str(depth)}:'),end='',flush=True)
startTime = time.time()
result = runCap(f'{cfg.BUILD_TEST_DIR}/perft {str(depth)}')
elapsedTime = time.time() - startTime
print(TIME_FORMATTER.format(elapsedTime))

if result.returncode != 0:
  print(f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}) {decodeOutput(result.stderr)}{util.RESET}')
  exit(2)
stats = decodeOutput(result.stdout).strip().split(' ')
resLeaves = int(stats[0])
resStates = int(stats[1])
resTerminals = int(stats[2])
print(f'Leaves: {resLeaves}')
print(f'States: {resStates}  ({resStates/elapsedTime:9,.0f} states/s)')
print(f'Terminals: {resTerminals}')
