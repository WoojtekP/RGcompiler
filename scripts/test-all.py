#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Run predefined tests.')
parser.add_argument('games', nargs='*', help='run tests only for these games')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)

args = parser.parse_args()
games = args.games
translateOptions = '"' + args.translateOptions + '"'

tests = {}
tests['ticTacToe.rg'] = ((100000,7.63,[64.84,35.16]), [1,9,72,504,3024,15120,54720]) # 148176 200448 127872
tests['ticTacToe.rbg'] = tests['ticTacToe.rg']

tests['breakthrough.rg'] = ((10000,64.10,[50.92,49.08]), [1,22,484,11132,256036,6182818]) # 149264638
tests['breakthrough.hrg'] = tests['breakthrough.rg']
tests['breakthrough.rbg'] = tests['breakthrough.rg']
tests['breakthroughWithAny.rg'] = tests['breakthrough.rg']

tests['hex2.rbg'] = ((1000,3.50,[50.00,50.00]), [1,4,12,24,12,0])

tests['hex9.rbg'] = ((1000,107.51,[52.30,47.70]), [1,81,6480]) # 511920 39929760

tests['connect4.hrg'] = ((10000,21.31,[55.72,44.28]), [1,7,49,343,2401,16807]) # 117649 823536 5673234

tests['amazons.hrg'] = ((200,71.46,[50.10,49.90]), [1,2176]) # 4307152
tests['amazons-naive.hrg'] = tests['amazons.hrg']
tests['amazons-smart.hrg'] = tests['amazons.hrg']

if len(games) == 0:
  games.append('ticTacToe.rg')
  games.append('ticTacToe.rbg')
  games.append('breakthroughWithAny.rg')
  games.append('breakthrough.rg')
  games.append('breakthrough.hrg')
  games.append('breakthrough.rbg')
  games.append('hex2.rbg')
  games.append('hex9.rbg')
  games.append('connect4.hrg')
  games.append('amazons-smart.hrg')
  games.append('amazons-naive.hrg')

print(f'Testing: {" ".join(games)}')
print(f'with translate options {translateOptions}')

HEAD_FORMATTER = '{: <30} '
RESULT_FORMATTER = '{: <20}{:9.3f} s'

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

  print(HEAD_FORMATTER.format(f'{game} compile:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'python3 scripts/compile.py {game} -t{translateOptions}')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    info = f'{util.ERROR} exitcode {result.returncode}'
  else:
    info = f'{util.OK}'
  print(RESULT_FORMATTER.format(info, elapsedTime))
  if result.returncode != 0:
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
    continue

  print(HEAD_FORMATTER.format(f'{game} g++:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'''
    g++ -c {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {cfg.GCC_FLAGS} -o {cfg.BUILD_TEST_DIR}/reasoner.o &&
    g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.o -I{cfg.BUILD_TEST_DIR} {cfg.GCC_FLAGS} -o {cfg.BUILD_TEST_DIR}/sims &&
    g++ test/perft.cpp {cfg.BUILD_TEST_DIR}/reasoner.o -I{cfg.BUILD_TEST_DIR} {cfg.GCC_FLAGS} -o {cfg.BUILD_TEST_DIR}/perft
  ''')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    info = f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}){util.RESET}'
  else:
    info = f'{util.OK}'
  print(RESULT_FORMATTER.format(info, elapsedTime))
  if result.returncode != 0:
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
    continue

  isOK = True

  sims = tests[game][0][0]
  avgDepth = tests[game][0][1]
  avgScores = tests[game][0][2]
  print(HEAD_FORMATTER.format(f'{game} sims {sims:}:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {sims}')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    info = f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}) {decodeOutput(result.stderr)}{util.RESET}'
    isOK = False
  else:
    expectedList = [avgDepth] + avgScores
    stats = decodeOutput(result.stdout).strip().split(' ')
    resStates = int(stats[0])
    resultList = [resStates / sims] # avgDepth
    stats = stats[6:]
    for p in range(len(stats)): resultList.append(int(stats[p]) / sims) # avgScores
    if not verifyWithTolerance(expectedList, resultList):
      info = f'{util.ERROR} expected {util.CYAN}{" ".join(f"{x:1.2f}" for x in expectedList)}{util.RESET} but got {util.CYAN}{" ".join(f"{x:1.2f}" for x in resultList)}{util.RESET}'
      isOK = False
    else:
      info = f'{util.OK}'
  print(RESULT_FORMATTER.format(info, elapsedTime))

  expectedPerft = tests[game][1]
  for depth in range(len(expectedPerft)):
    print(HEAD_FORMATTER.format(f'{game} perft {depth}:'),end='',flush=True)
    startTime = time.time()
    result = runCap(f'{cfg.BUILD_TEST_DIR}/perft {depth}')
    elapsedTime = time.time() - startTime
    if result.returncode != 0:
      info = f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}) {decodeOutput(result.stderr)}{util.RESET}'
      isOK = False
    else:
      stats = decodeOutput(result.stdout).strip().split(' ')
      resLeaves = int(stats[0])
      if expectedPerft[depth] != resLeaves:
        info = f'{util.ERROR} expected {util.CYAN}{expectedPerft[depth]}{util.RESET} but got {util.CYAN}{resLeaves}{util.RESET}'
      else:
        info = f'{util.OK}'
    print(RESULT_FORMATTER.format(info, elapsedTime))

  if isOK: gamesOK.append(game)

totalElapsedTime = time.time() - totalStartTime
gamesError = [game for game in games if game not in gamesOK]

print()
print((HEAD_FORMATTER+RESULT_FORMATTER).format(f'--- Summary --- {util.GREEN}{util.RESET}', '', totalElapsedTime))
print(f'Games {util.OK}: {" ".join(gamesOK)}')
if len(gamesError) == 0:
  print(f'No errors.')
else:
  print(f'Games with {util.ERROR}: {" ".join(gamesError)}')
