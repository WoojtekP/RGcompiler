#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile and run perft.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('depth', nargs=1, help='perft depth')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)

args = parser.parse_args()
game = args.game[0]
depth = args.depth[0]
translateOptions = '"' + args.translateOptions + '"'

run(f'python3 scripts/compile.py {game} -t{translateOptions}')

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
#print(stats)
resLeaves = int(stats[0])
resStates = int(stats[1])
resTerminals = int(stats[2])
print(f'leaves: {resLeaves}')
print(f'states: {resStates}  ({resStates/elapsedTime:9,.3f} states/s)')
print(f'terminals: {resTerminals}')
