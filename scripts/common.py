import sys, os, subprocess, time
from shutil import which

def run(cmd):
  result = subprocess.run(cmd, shell=True)
  if result.returncode != 0:
    print(f'exitcode {result.returncode} for {cmd}')
    exit(2)

def buildInterpreter():
  #run(f'cargo run --release --manifest-path {cfg.RG_DIR}/interpreter_rust/Cargo.toml --help >/dev/null 2>/dev/null')
  run(f'cargo run --release --manifest-path {cfg.RG_DIR}/interpreter_rust/Cargo.toml --help')

def runCap(cmd):
  return subprocess.run(cmd, shell=True, capture_output=True)

def isProgramAvailable(name):
  return which(name) is not None

def decodeOutput(output):
  return output.decode('UTF-8')

def parseGameName(game):
  nameWithExt = game.split('.')
  if len(nameWithExt) == 1:
    print(f'Invalid game name (no extension): {game}')
    return None
  if len(nameWithExt) > 2:
    print(f'Invalid game name (too many dots): {game}')
    return None
  baseName = nameWithExt[0].split('-')[0]
  if nameWithExt[1] == 'py': gameFile = 'py/hrg/' + nameWithExt[0] + '.hrg'
  else: gameFile = nameWithExt[1] + '/' + game
  return (baseName, gameFile)

class cfg:
  BUILD_DIR = 'build'
  BUILD_TEST_DIR = 'build-test'
  RG_DIR = '../rg'

  DEFAULT_TRANSLATE_OPTIONS = '--enable-all-optimizations --enable-all-pragmas'
  #DEFAULT_TRANSLATE_OPTIONS += '--calculate-disjoints --calculate-iterators --calculate-repeats-and-uniques --calculate-simple-apply --calculate-tag-indexes'
  #DEFAULT_TRANSLATE_OPTIONS = ' --compact-comparisons --compact-skip-edges --inline-assignment --inline-reachability --join-exclusive-edges --join-fork-prefixes --join-fork-suffixes --merge-accesses --propagate-constants --prune-singleton-types --prune-unreachable-nodes --prune-unused-constants --prune-unused-variables --skip-artificial-tags --skip-redundant-tags --skip-self-assignments --skip-self-comparisons --skip-unused-tags'
  DEFAULT_RG2CPP_OPTIONS = '--simple-path 0 --disjoint 1 --all-unique 0 --arithmetic 1 --max-move-len -1 --gccinline 0  --support-apply-move-for-keeper 1 --remove-unecessary-funcion-arguments 1 --remove-unecessary-funcions 1'
  DEBUG_RG2CPP_OPTIONS = '--no-cycle-detection 0 --print-function-names 0 --preserve-original-node-names 1 --verification 0'

  result = runCap('g++ --version')
  if 'clang' in decodeOutput(result.stdout):
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++23 -Ofast -flto'
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++23 -Ofast -flto -DNDEBUG'
  else:
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++23 -Ofast -flto=auto -ftracer --param max-inline-insns-auto=1024 -march=native -DNDEBUG -s'
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++23 -Ofast -flto=auto --param max-inline-insns-auto=1024 -march=native -ftracer'
    GCC_DEBUG_FLAGS = '-Wall -Wextra -std=c++23 -g -Og -ggdb3 -march=native -ftracer -fsanitize=address -static-libasan -fno-omit-frame-pointer -fsanitize=undefined'
    GCC_PROFILE_FLAGS = '-Wall -Wextra -std=c++23 -Og -pg -march=native -ftracer'

def createAST(game, translateFlags, silent):
  parsed = parseGameName(game)
  if parsed == None: return f'Cannot parse {game}'
  (gameName,gameFile) = parsed
  if not os.path.isfile(f'{cfg.RG_DIR}/games/{gameFile}'): return f'There is no file {cfg.RG_DIR}/games/{gameFile}'

  if not silent: print(f'Preparing {game} with options {translateFlags}')
  startTime = time.time()
  tmp_ast_file = f'{cfg.BUILD_TEST_DIR}/{game}.json.tmp'
  result = runCap(f'cargo run --release --manifest-path {cfg.RG_DIR}/interpreter_rust/Cargo.toml ast {translateFlags} {cfg.RG_DIR}/games/{gameFile} > {tmp_ast_file}')
  if result.returncode != 0: return decodeOutput(result.stderr).strip()
  result = runCap(f'python3 scripts/adjust_AST.py {tmp_ast_file} {cfg.BUILD_TEST_DIR}/{game}.json')
  if result.returncode != 0: return decodeOutput(result.stderr).strip()
  run(f'rm {tmp_ast_file}')
  elapsedTime = time.time() - startTime
  if not silent: print(FORMATTER.format("ast:",elapsedTime))
  
def rg2cpp(game, compileFlags, silent):
  if not silent:
    print(f'Compiling {game} with options "{compileFlags}"')
    startTime = time.time()
  os.chdir(cfg.BUILD_TEST_DIR)
  result = runCap(f'../{cfg.BUILD_DIR}/src/rg2cpp --file {game}.json -o reasoner {compileFlags}')
  os.chdir('..')
  if result.returncode != 0: return decodeOutput(result.stderr).strip()
  if not silent:
    elapsedTime = time.time() - startTime
    print(FORMATTER.format("rg2cpp:",elapsedTime))

def compileCpp(main, compiler, flags):
  result = runCap(f'{compiler} test/{main}.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {flags} -o {cfg.BUILD_TEST_DIR}/{main}')
  if result.returncode != 0: return decodeOutput(result.stderr).strip()

class util:
  RESET = "\033[0m"
  RED = "\033[31m"
  GREEN = "\033[32m"
  CYAN = "\033[36m"
  YELLOW = "\033[33m"

  ERROR = f'{RED}ERROR{RESET}'
  OK = f'{GREEN}OK{RESET}'
