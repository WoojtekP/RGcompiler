#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run perft.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('depth', nargs=1, help='perft depth')
parser.add_argument('-t', dest='translateFlags', nargs='?', help='translate flags for interpreter_rust', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('--reuse', action='store_true', help='reuse already built reasoner (no translation nor rg2cpp)')
parser.add_argument('--perf', action='store_true', help='count instructions by perf')
parser.add_argument('--clang', action='store_true', help='use clang++ instead of g++')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('--benchmark', action='store_true', help='maximum speed flags (default)')
cpp_flags_group.add_argument('--test', action='store_true', help='optimization and asserts')
cpp_flags_group.add_argument('--debug', action='store_true', help='gdb symbols and sanitizers')
cpp_flags_group.add_argument('--profile', action='store_true', help='generate profiler information')

args = parser.parse_args()
game = args.game[0]
depth = args.depth[0]

translateFlags = args.translateFlags
benchmarkMode = False
if args.profile:
  cppFlags = cfg.GCC_PROFILE_FLAGS
  mode = "profile"
elif args.debug:
  cppFlags = cfg.GCC_DEBUG_FLAGS
  mode = "debug"
elif args.test:
  cppFlags = cfg.GCC_TEST_FLAGS
  mode = "test"
else:
  cppFlags = cfg.GCC_BENCHMARK_FLAGS
  mode = "benchmark"
  benchmarkMode = True

if args.clang: compiler = 'clang++'
else: compiler = 'g++'

print(f'Mode {util.GREEN}{mode}{util.RESET}, limit: ',end='')
print(f'Depth {depth}')
print(f'Game: {game}')
print(f'Translate flags: {translateFlags}')
print(f'rg2cpp flags: {cfg.DEFAULT_RG2CPP_OPTIONS}')
print(f'{compiler} flags: {cppFlags}')
print()

#######################################################################################################################

HEAD_FORMATTER = '{: <14} '
TIME_FORMATTER = '{:9.3f} s'
FULL_FORMATTER = HEAD_FORMATTER + TIME_FORMATTER

while True:
  print(HEAD_FORMATTER.format(f'{game}:'),end='',flush=True)
  
  ######## Translate ########
  if not args.reuse:
    print(f' | ast ',end='',flush=True)
    startTime = time.time()
    error = createAST(game, translateFlags, True)
    if error != None:
      print(f'\n{util.ERROR} {util.CYAN}{error}{util.RESET}')
      break
    elapsedTime = time.time() - startTime
    print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)
    
    print(f' | rg2cpp ',end='',flush=True)
    startTime = time.time()
    result = rg2cpp(game, cfg.DEFAULT_RG2CPP_OPTIONS + ' ' + cfg.DEBUG_RG2CPP_OPTIONS, True)
    if error != None:
      print(f'\n{util.ERROR} {util.CYAN}{error}{util.RESET}')
      break
    print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)
      
  ######## Compile cpp ########
  print(f' | {compiler} ',end='',flush=True)
  startTime = time.time()
  error = compileCpp('perft', compiler, f'{cppFlags}')
  elapsedTime = time.time() - startTime
  if error != None:
    print(f'{util.ERROR} {util.CYAN}{error}{util.RESET}')
    break
  print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)

  ######## Perft ########
  print(HEAD_FORMATTER.format(f' | perft {str(depth)}'),end='',flush=True)
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
  
  break
