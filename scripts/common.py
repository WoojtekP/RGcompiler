import subprocess
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
  gameFile = nameWithExt[1] + '/' + game
  return (baseName, gameFile)

class cfg:
  BUILD_DIR = 'build'
  BUILD_TEST_DIR = 'build-test'
  RG_DIR = '../rg'

  DEFAULT_TRANSLATE_OPTIONS = '--enable-all-optimizations --enable-all-pragmas'
  #DEFAULT_TRANSLATE_OPTIONS = '--enable-all-optimizations --calculate-disjoints --calculate-repeats-and-uniques --calculate-tag-indexes' # No simpleApply
  #DEFAULT_TRANSLATE_OPTIONS = ' --enable-all-pragmas'
  #DEFAULT_TRANSLATE_OPTIONS += ' --compact-comparisons --compact-skip-edges --inline-assignment --inline-reachability --join-exclusive-edges --join-fork-prefixes --join-fork-suffixes --merge-accesses --propagate-constants --prune-singleton-types --prune-unreachable-nodes --prune-unused-constants --prune-unused-variables --skip-artificial-tags --skip-self-assignments --skip-self-comparisons --skip-unused-tags'
  DEFAULT_RG2CPP_OPTIONS = '--simple-path 0 --disjoint 1 --all-unique 0 --max-move-len -1'
  DEBUG_RG2CPP_OPTIONS = '--no-cycle-detection 0 --print-function-names 0 --preserve-original-node-names 1 --verification 0 --gccinline 0'

  result = runCap('g++ --version')
  if 'clang' in decodeOutput(result.stdout):
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto'
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto -DNDEBUG'
  else:
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto=auto -march=native -ftracer -DNDEBUG -s'
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto=auto -march=native -ftracer'
    GCC_DEBUG_FLAGS = '-Wall -Wextra -std=c++20 -g -Og -ggdb3 -march=native -ftracer'
    GCC_DEBUG_FLAGS += ' -fsanitize=address -static-libasan -fno-omit-frame-pointer -fsanitize=undefined'
    GCC_PROFILE_FLAGS = '-Wall -Wextra -std=c++20 -Og -pg -march=native -ftracer'
    # -finline-limit=100

class util:
  RESET = "\033[0m"
  RED = "\033[31m"
  GREEN = "\033[32m"
  CYAN = "\033[36m"
  YELLOW = "\033[33m"

  ERROR = f'{RED}ERROR{RESET}'
  OK = f'{GREEN}OK{RESET}'
