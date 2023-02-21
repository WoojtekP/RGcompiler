#!/bin/python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run perft.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('depth', nargs=1, help='perft depth')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=[cfg.DEFAULT_TRANSLATE_OPTIONS])

args = parser.parse_args()
game = args.game[0]
depth = args.depth[0]
translateOptions = '"' + args.translateOptions + '"'

run(f'python3 scripts/compile.py {game} -t{translateOptions}')

FORMATTER = "{: <15}{:9.3f} s"

startTime = time.time()
run(f'g++ test/perft.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {cfg.GCC_FLAGS} -o {cfg.BUILD_TEST_DIR}/perft')
elapsedTime = time.time() - startTime
print(FORMATTER.format('g++:',elapsedTime))

print(f'Running perft depth {str(depth)}')

startTime = time.time()
result = runCap(f'{cfg.BUILD_TEST_DIR}/perft {str(depth)}')
elapsedTime = time.time() - startTime
print(FORMATTER.format(f'perft depth {str(depth)}:',elapsedTime))

if result.returncode != 0:
  print("ERROR: (exitCode "+result.returncode+") "+result.stderr)
  exit(1)
stats = result.stdout.decode('UTF-8').strip().split(' ')
#print(stats)
resLeaves = int(stats[0])
resStates = int(stats[1])
resTerminals = int(stats[2])
print()
print(f'leaves: {resLeaves}')
print(f'states: {resStates}  ({resStates/elapsedTime:9,.3f} states/s)')
print(f'terminals: {resTerminals}')
