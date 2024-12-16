#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Compile a game to C++ reasoner.')
parser.add_argument('game', help='game file')
parser.add_argument('-o', dest='outputFile', help='name of output file with generated code', default="reasoner")
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-c', dest='compileOptions', nargs='?', help='compile options for rg2cpp', default=cfg.DEFAULT_RG2CPP_OPTIONS)

args = parser.parse_args()
game = args.game
translateOptions = args.translateOptions
compileOptions = args.compileOptions
outputFile = args.outputFile

if not os.path.isfile(f'{cfg.RG_DIR}/games/{game}'):
  print(f'There is no file {cfg.RG_DIR}/games/{game}', file=sys.stderr)
  exit(2)

game_basename = game.replace("/","_").split('.')[0]

print(f'Preparing {game} with options "{translateOptions}"')

run("mkdir -p "+cfg.BUILD_TEST_DIR)
run("cp defaultMap.hpp "+cfg.BUILD_TEST_DIR+"/defaultMap.hpp")

FORMATTER = "{: <15}{:9.3f} s"

# Create AST
startTime = time.time()
tmp_ast_file = f"{cfg.BUILD_TEST_DIR}/{game_basename}.json"
#run(f"node {cfg.RG_DIR}/interpreter_node/lib/cli {translateOptions} rg-ast {cfg.RG_DIR}/examples/{game} > {tmp_ast_file}")
run(f"cargo run --release --manifest-path {cfg.RG_DIR}/interpreter_rust/Cargo.toml ast {translateOptions} {cfg.RG_DIR}/games/{game} > {tmp_ast_file}")
run(f"python3 scripts/adjust_AST.py {tmp_ast_file} {cfg.BUILD_TEST_DIR}/{game_basename}-ast.json")
run(f"rm {tmp_ast_file}")
elapsedTime = time.time() - startTime
print(FORMATTER.format("ast:",elapsedTime))

# Generate cpp files
print(f'Compiling {game} with options "{compileOptions}"')
startTime = time.time()
os.chdir(cfg.BUILD_TEST_DIR)
run(f'../{cfg.BUILD_DIR}/src/rg2cpp --file {game_basename}-ast.json -o {outputFile} {cfg.DEFAULT_RG2CPP_OPTIONS} {cfg.DEBUG_RG2CPP_OPTIONS}')
elapsedTime = time.time() - startTime
print(FORMATTER.format("rg2cpp:",elapsedTime))

# Format generated files
if isProgramAvailable('clang-format'):
  startTime=time.time()
  run(f"clang-format -style=file -i {outputFile}.hpp {outputFile}.cpp")
  elapsedTime = time.time() - startTime
  print(FORMATTER.format('clang-format:',elapsedTime))
else:
  print(f'clang-format: omitted because unavailable')
