#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run simulations.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('sims', nargs=1, help='number of simulations')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-profile', action='store_true', help='generate profiler information')
parser.add_argument('-skipcompilation', action='store_true', help='skip compile.py and use the existing reasoner sources')

args = parser.parse_args()
game = args.game[0]
sims = int(args.sims[0])
translateOptions = '"' + args.translateOptions + '"'

if not args.skipcompilation:
  run(f'python3 scripts/compile.py {game} -t{translateOptions}')

HEAD_FORMATTER = '{: <14} '
TIME_FORMATTER = '{:9.3f} s'
FULL_FORMATTER = HEAD_FORMATTER + TIME_FORMATTER

if args.profile:
  gccFlags = cfg.GCC_PROFILE_FLAGS
else:
  gccFlags = cfg.GCC_TEST_FLAGS
  
startTime = time.time()
run(f'g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {gccFlags} -o {cfg.BUILD_TEST_DIR}/sims')
elapsedTime = time.time() - startTime
print(FULL_FORMATTER.format('g++:',elapsedTime))

print(HEAD_FORMATTER.format(f'sims {str(sims)}:'),end='',flush=True)
startTime = time.time()
result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {str(sims)}')
elapsedTime = time.time() - startTime
print(TIME_FORMATTER.format(elapsedTime))

if result.returncode != 0:
  print(f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}) {decodeOutput(result.stderr)}{util.RESET}')
  exit(2)
stats = decodeOutput(result.stdout).strip().split(' ')
#print(stats)
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
print(f'states: {resStates}  ({resStates / elapsedTime:9,.3f} states/s; {sims / elapsedTime:9,.3f} sims/s)')
print(f'depth: min {resMinDepth} avg {resAvgDepth:1.2f} max {resMaxDepth}')
print(f'scores: avg {" ".join(f"{avgScore:1.2f}" for avgScore in resAvgScores)}')

if args.profile:
  run(f'gprof {cfg.BUILD_TEST_DIR}/sims gmon.out > gmon.txt')
