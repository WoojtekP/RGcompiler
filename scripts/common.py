import subprocess
from shutil import which

def run(cmd):
  result = subprocess.run(cmd, shell=True)
  if result.returncode != 0:
    print(f'exitcode {result.returncode} for {cmd}')
    exit(2)

def runCap(cmd):
  return subprocess.run(cmd, shell=True, capture_output=True)

def isProgramAvailable(name):
  return which(name) is not None

def decodeOutput(output):
  return output.decode('UTF-8')

class cfg:
  BUILD_DIR = 'build'
  BUILD_TEST_DIR = 'build-test'
  RG_DIR = '../rg'

  DEFAULT_TRANSLATE_OPTIONS = '--expandGeneratorNodes --compactSkipEdges --reuseFunctions --normalizeTypes --addExplicitCasts'
  DEFAULT_TRANSLATE_OPTIONS += ' --calculateUniques --calculateTagIndexes' # Auto-optimization
  DEFAULT_RG2CPP_OPTIONS = '--opt-conditions 3 --simple-path 1'

  result = runCap('g++ --version')
  if 'clang' in decodeOutput(result.stdout):
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++17 -Ofast -flto'
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++17 -Ofast -flto -DNDEBUG'
  else:
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++17 -Ofast -flto=auto -march=native -ftracer'
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++17 -Ofast -flto=auto  -march=native -ftracer -DNDEBUG -finline-limit=100'

class util:
  RESET = "\033[0m"
  RED = "\033[31m"
  GREEN = "\033[32m"
  CYAN = "\033[36m"
  YELLOW = "\033[33m"

  ERROR = f'{RED}ERROR{RESET}'
  OK = f'{GREEN}OK{RESET}'
