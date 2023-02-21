#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run simulations.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('sims', nargs=1, help='number of simulations')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)

args = parser.parse_args()
game = args.game[0]
sims = int(args.sims[0])
translateOptions = '"' + args.translateOptions + '"'

run(f'python3 scripts/compile.py {game} -t{translateOptions}')

FORMATTER = "{: <15}{:9.3f} s"

startTime = time.time()
run(f'g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {cfg.GCC_FLAGS} -o {cfg.BUILD_TEST_DIR}/sims')
elapsedTime = time.time() - startTime
print(FORMATTER.format('g++:',elapsedTime))

print(f'Running simulations with count {str(sims)}')

startTime = time.time()
result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {str(sims)}')
elapsedTime = time.time() - startTime
print(FORMATTER.format(f'sims {str(sims)}:',elapsedTime))

if result.returncode != 0:
  print("ERROR: (exitCode "+result.returncode+") "+result.stderr)
  exit(1)
stats = result.stdout.decode('UTF-8').strip().split(' ')
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

print()
print(f'states: {resStates}  ({resStates / elapsedTime:9,.3f} states/s)')
print(f'depth: min {resMinDepth} avg {resAvgDepth:1.2f} max {resMaxDepth}')
print(f'scores: avg {" ".join(f"{avgScore:1.2f}" for avgScore in resAvgScores)}')
