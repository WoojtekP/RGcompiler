#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Run predefined validation tests for given games.')
parser.add_argument('game', nargs='+', help='run tests for these games (use \"all\" for all default predefined tests')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-q', '--quiet', action='store_true', help='suppress g++ warnings')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('-debug', action='store_true', help='compile with gdb symbols')
cpp_flags_group.add_argument('-profile', action='store_true', help='generate profiler information')
cpp_flags_group.add_argument('-benchmark', action='store_true', help='maximum speed flags')

args = parser.parse_args()
games = args.game
translateOptions = args.translateOptions
if args.profile:
  gccOptions = cfg.GCC_PROFILE_FLAGS
  infoGccOptions = "profile"
elif args.debug:
  gccOptions = cfg.GCC_DEBUG_FLAGS
  infoGccOptions = "debug"
elif args.benchmark:
  gccOptions = cfg.GCC_BENCHMARK_FLAGS
  infoGccOptions = "benchmark"
else:
  gccOptions = cfg.GCC_TEST_FLAGS
  infoGccOptions = "test"


#######################################################################################################################
# (sims, avgDepth, [avgScore0,...], [perft0,perft1,...]

tests = {}
tests['ticTacToe.rg'] = (100000,7.63,[64.84,35.16], [1,9,72,504,3024,15120,54720]) # 148176 200448 127872
tests['ticTacToe.rbg'] = tests['ticTacToe.rg']

tests['breakthrough.rg'] = (10000,64.10,[50.92,49.08], [1,22,484,11132,256036,6182818]) # 149264638
tests['breakthrough.hrg'] = tests['breakthrough.rg']
tests['breakthrough.rbg'] = tests['breakthrough.rg']

tests['connect4.hrg'] = (10000,21.31,[55.72,44.28], [1,7,49,343,2401,16807]) # 117649 823536 5673234

tests['hex2.rbg'] = (1000,3.50,[50.00,50.00], [1,4,12,24,12,0])

tests['hex9.rbg'] = (1000,71.02,[53.03,46.97], [1,81,6480]) # 511920 39929760

tests['knightthrough.hrg'] = (10000,33.64,[51.67,48.33], [1,40,1600,63520,2521306,99598454]) # 3929482778

tests['amazons.hrg'] = (200,71.46,[50.10,49.90], [1,2176]) # 4307152

tests['repeatTest.rg'] = (100,1.0,[100.0], [1,1,0])
tests['repeatTestBig.rg'] = (1000,1.0,[100.0], [1,2,0])
tests['repeatTestHard.rg'] = (1000,1.0,[12.5], [1,16,0])
tests['simpleApplyTest0.rg'] = (1000,1.0,[0.0], [1,2,0])
tests['simpleApplyTest1.rg'] = (1000,2.0,[0.0,50.0], [1,2,4,0])
tests['simpleApplyTest2.rg'] = (1000,2.0,[0.0,100.0], [1,2,2,0])
tests['simpleApplyTest3.rg'] = (1000,2.0,[0.0,75.0], [1,2,3,0])
tests['simpleApplyTest4.rg'] = (1000,2.0,[0.0,50.0], [1,1,3,0])
tests['simpleApplyTest5.rg'] = (1000,1.0,[0.0,50.0], [1,2,0])
tests['simpleApplyTest6.rg'] = (1000,2.0,[0.0,50.0], [1,3,5,0])


if "all" in games:
  games = []
  games.append('ticTacToe.rg')
  games.append('breakthrough.rg')

  games.append('repeatTest.rg')
  games.append('repeatTestBig.rg')
  games.append('repeatTestHard.rg')

  games.append('simpleApplyTest0.rg')
  games.append('simpleApplyTest1.rg')
  games.append('simpleApplyTest2.rg')
  games.append('simpleApplyTest3.rg')
  games.append('simpleApplyTest4.rg')
  games.append('simpleApplyTest5.rg')
  games.append('simpleApplyTest6.rg')

  #games.append('ticTacToe.rbg')
  #games.append('breakthrough.hrg')
  #games.append('breakthrough.rbg')
  #games.append('connect4.hrg')
  #games.append('hex2.rbg')
  #games.append('hex9.rbg')
  #games.append('knightthrough.hrg')
  #games.append('amazons-smart.hrg')
  #games.append('amazons-naive.hrg')

print(f'Testing #{len(games)}: {" ".join(games)}')
print(f'Translate options: {translateOptions}')
print(f'rbg2cpp options: {cfg.DEFAULT_RG2CPP_OPTIONS}')
print(f'g++ {infoGccOptions} options: {gccOptions}')

#######################################################################################################################

HEAD_FORMATTER = '{: <50} '
RESULT_FORMATTER = '{: <20}{:9.3f} s'
STAT_FORMATTER = '  {:15,.3f} states/s'

def printResult(info, elapsedTime, count=0):
  print(RESULT_FORMATTER.format(info, elapsedTime) + ("" if count == 0 else STAT_FORMATTER.format(count/elapsedTime).replace(',',' ')))

TOLERANCE = 0.1

def verifyWithTolerance(expectedList, resultList):
  if len(expectedList) != len(resultList): return False
  for i in range(len(expectedList)):
    if abs(expectedList[i] - resultList[i]) > expectedList[i] * TOLERANCE:
      return False
  return True

gamesOK = []
totalStartTime = time.time()
for game in games:
  print()

  nameExt = game.split('.')
  baseName = game.split('-')[0]
  gameFile = nameExt[1] + '/' + game
  if game in tests:
    gameRef = game
  else:
    gameRef = baseName + '.' + nameExt[1]
    if gameRef not in tests:
      print(f'Not matched tests for game {game}')
      continue

  ######## Compile ########
  print(HEAD_FORMATTER.format(f'{game} compile:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'python3 scripts/compile.py {gameFile} -t"{translateOptions}"')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    info = f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}'
  else:
    info = f'{util.OK}'
  printResult(info, elapsedTime)
  if result.returncode != 0:
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
    continue
  print(HEAD_FORMATTER.format(f'{game} g++:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'''
    g++ -c {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {cfg.GCC_TEST_FLAGS} -o {cfg.BUILD_TEST_DIR}/reasoner.o &&
    g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.o -I{cfg.BUILD_TEST_DIR} {cfg.GCC_TEST_FLAGS} -o {cfg.BUILD_TEST_DIR}/sims &&
    g++ test/perft.cpp {cfg.BUILD_TEST_DIR}/reasoner.o -I{cfg.BUILD_TEST_DIR} {cfg.GCC_TEST_FLAGS} -o {cfg.BUILD_TEST_DIR}/perft
  ''')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    info = f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}'
  else:
    info = f'{util.OK}'
  printResult(info, elapsedTime)
  if not args.quiet:
    errOutput = decodeOutput(result.stderr).strip()
    if errOutput != "":
      print(f'{util.CYAN}{errOutput}{util.RESET}')
  if result.returncode != 0: continue

  isOK = True

  ######## Sims ########
  sims = tests[gameRef][0]
  avgDepth = tests[gameRef][1]
  avgScores = tests[gameRef][2]
  print(HEAD_FORMATTER.format(f'{game} sims {sims:}:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {sims}')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    info = f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}'
    isOK = False
    errInfo = decodeOutput(result.stderr).strip()
    resStates = 0
  else:
    expectedList = [avgDepth] + avgScores
    stats = decodeOutput(result.stdout).strip().split(' ')
    resSims = int(stats[0])
    resStates = int(stats[1])
    resultList = [resStates / sims] # avgDepth
    stats = stats[7:]
    for p in range(len(stats)): resultList.append(int(stats[p]) / sims) # avgScores
    if not verifyWithTolerance(expectedList, resultList):
      info = f'{util.ERROR} expected {util.CYAN}{" ".join(f"{x:1.2f}" for x in expectedList)}{util.RESET} but got {util.CYAN}{" ".join(f"{x:1.2f}" for x in resultList)}{util.RESET}'
      isOK = False
    else:
      info = f'{util.OK}'
    errInfo = None
  printResult(info, elapsedTime, resStates)
  if errInfo != None: print(f'{util.CYAN}{errInfo}{util.RESET}')

  ######## Perft ########
  expectedPerft = tests[gameRef][3]
  for depth in range(len(expectedPerft)):
    print(HEAD_FORMATTER.format(f'{game} perft {depth}:'),end='',flush=True)
    startTime = time.time()
    result = runCap(f'{cfg.BUILD_TEST_DIR}/perft {depth}')
    elapsedTime = time.time() - startTime
    if result.returncode != 0:
      info = f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}'
      errInfo = decodeOutput(result.stderr).strip()
      isOK = False
      resStates = 0
    else:
      stats = decodeOutput(result.stdout).strip().split(' ')
      resLeaves = int(stats[0])
      resStates = int(stats[1])
      if expectedPerft[depth] != resLeaves:
        info = f'{util.ERROR} expected {util.CYAN}{expectedPerft[depth]}{util.RESET} but got {util.CYAN}{resLeaves}{util.RESET}'
      else:
        info = f'{util.OK}'
      errInfo = None
    printResult(info, elapsedTime, resStates)
    if errInfo != None: print(f'{util.CYAN}{errInfo}{util.RESET}')

  if isOK: gamesOK.append(game)

totalElapsedTime = time.time() - totalStartTime
gamesError = [game for game in games if game not in gamesOK]

print()
print((HEAD_FORMATTER+RESULT_FORMATTER).format(f'--- Summary --- {util.GREEN}{util.RESET}', '', totalElapsedTime))
print(f'Games with {util.OK}: {" ".join(gamesOK)}')
if len(gamesError) == 0:
  print(f'No errors.')
else:
  print(f'Games with {util.ERROR}: {" ".join(gamesError)}')
