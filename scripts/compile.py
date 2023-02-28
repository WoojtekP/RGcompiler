#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile a game to C++ reasoner.')
parser.add_argument('game', nargs=1, help='game file')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)

args = parser.parse_args()
game = args.game[0]
translateOptions = args.translateOptions

if not os.path.isfile(f'{cfg.RG_DIR}/examples/{game}'):
  print(f'There is no file {cfg.RG_DIR}/examples/{game}', file=sys.stderr)
  exit(2)

print(f'Compiling {game} with translate options "{translateOptions}"')

run("mkdir -p "+cfg.BUILD_TEST_DIR)
run("cp defaultMap.hpp "+cfg.BUILD_TEST_DIR+"/defaultMap.hpp")

FORMATTER = "{: <15}{:9.3f} s"

# Create AST
startTime = time.time()
run(f"node {cfg.RG_DIR}/interpreter_node/lib/cli rg-source {cfg.RG_DIR}/examples/{game} > {cfg.BUILD_TEST_DIR}/game-tmp.rg")
run(f"node {cfg.RG_DIR}/interpreter_node/lib/cli {translateOptions} rg-ast {cfg.BUILD_TEST_DIR}/game-tmp.rg > {cfg.BUILD_TEST_DIR}/{game}.json")
run(f"python3 -m json.tool {cfg.BUILD_TEST_DIR}/{game}.json > {cfg.BUILD_TEST_DIR}/{game}-ast.json")
run(f"rm {cfg.BUILD_TEST_DIR}/game-tmp.rg")
elapsedTime = time.time() - startTime
print(FORMATTER.format("ast:",elapsedTime))

# Generate cpp files
startTime = time.time()
os.chdir(cfg.BUILD_TEST_DIR)
run(f'../{cfg.BUILD_DIR}/rg2cpp --file {game}-ast.json {cfg.DEFAULT_RG2CPP_OPTIONS}')
elapsedTime = time.time() - startTime
print(FORMATTER.format("rg2cpp:",elapsedTime))

# Format generated files
if isProgramAvailable('clang-format'):
  startTime=time.time()
  run("clang-format -style=file -i reasoner.hpp reasoner.cpp")
  elapsedTime = time.time() - startTime
  print(FORMATTER.format('clang-format:',elapsedTime))
else:
  print(f'clang-format: omitted because unavailable')

