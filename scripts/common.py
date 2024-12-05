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

  DEFAULT_TRANSLATE_OPTIONS = '--x-enable-all-optimizations --x-enable-all-pragmas'
  #DEFAULT_TRANSLATE_OPTIONS = '--addExplicitCasts --compactSkipEdges --inlineAssignment --inlineReachability --joinForkPrefixes --joinForkSuffixes --mangleSymbols --pruneSingletonTypes --pruneUnreachableNodes --pruneUnusedConstants --pruneUnusedVariables --reuseFunctions --skipGeneratorComparisons --skipSelfAssignments --skipSelfComparisons --skipUnusedTags'
  #DEFAULT_TRANSLATE_OPTIONS += ' --calculateDisjoints --calculateRepeats --calculateSimpleApply --calculateTagIndexes --calculateUniques ' # Auto-optimization
  DEFAULT_RG2CPP_OPTIONS = '--simple-path 0 --disjoint 1'
  DEBUG_RG2CPP_OPTIONS = '--no-cycle-detection 0 --print-function-names 0 --preserve-original-node-names 1 --verification 0 --gccinline 0'

  result = runCap('g++ --version')
  if 'clang' in decodeOutput(result.stdout):
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto'
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto -DNDEBUG'
  else:
    GCC_TEST_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto=auto -march=native -ftracer'
    GCC_PROFILE_FLAGS = '-Wall -Wextra -std=c++20 -Og -pg -march=native -ftracer'
    GCC_BENCHMARK_FLAGS = '-Wall -Wextra -std=c++20 -Ofast -flto=auto -march=native -ftracer -DNDEBUG -s'
    # -finline-limit=100

class utils:
  RESET = "\033[0m"
  RED = "\033[31m"
  GREEN = "\033[32m"
  CYAN = "\033[36m"
  YELLOW = "\033[33m"

  ERROR = f'{RED}ERROR{RESET}'
  OK = f'{GREEN}OK{RESET}'
