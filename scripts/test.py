#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Run predefined validation tests for given games.')
parser.add_argument('game', nargs='+', help='run tests for these games (use \"all\" for all default predefined tests and \"short\" for a subset of games for quick test')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-q', '--quiet', action='store_true', help='suppress g++ warnings')
parser.add_argument('--clang', action='store_true', help='use clang++ instead of g++')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('--test', action='store_true', help='optimization and asserts (default)')
cpp_flags_group.add_argument('--debug', action='store_true', help='gdb symbols and sanitizers')
cpp_flags_group.add_argument('--profile', action='store_true', help='generate profiler information')

args = parser.parse_args()
games = args.game
translateOptions = args.translateOptions
if args.profile:
  cppFlags = cfg.GCC_PROFILE_FLAGS
  infoGccOptions = "profile"
elif args.debug:
  cppFlags = cfg.GCC_DEBUG_FLAGS
  infoGccOptions = "debug"
else:
  cppFlags = cfg.GCC_TEST_FLAGS
  infoGccOptions = "test"

compiler = 'clang++' if args.clang else 'g++'

#######################################################################################################################
# (sims, avgDepth, [avgScore0,...], [perft0,perft1,...]

tests = {}

tests['repeatTest'] = (100,1.0,[100.0], [1,1,0])
tests['repeatTestBig'] = (1000,1.0,[100.0], [1,2,0])
tests['repeatTestHard'] = (1000,1.0,[12.5], [1,16,0])
tests['simpleApplyTest0'] = (1000,1.0,[0.0], [1,2,0])
tests['simpleApplyTest1'] = (1000,2.0,[0.0,50.0], [1,2,4,0])
tests['simpleApplyTest2'] = (1000,2.0,[0.0,100.0], [1,2,2,0])
tests['simpleApplyTest3'] = (1000,2.0,[0.0,75.0], [1,2,3,0])
tests['simpleApplyTest4'] = (1000,2.0,[0.0,50.0], [1,1,3,0])
tests['simpleApplyTest5'] = (1000,1.0,[50.0], [1,2,0])
tests['simpleApplyTest6'] = (1000,2.0,[0.0,50.0], [1,3,5,0])
tests['simpleApplyDoubleTest'] = (1000,1.0,[28.56], [1,7,0])
tests['simpleApplyDoubleRevTest'] = (1000,1.0,[28.56], [1,7,0])
tests['simpleApplyDoubleSameTest'] = (1000,1.0,[28.56], [1,7,0])
tests['chessTest1'] = (1000,4.0,[50.0,50.0], [1,7,28,256,1664])
tests['alquerque'] = (100000,35.91,[54.88,45.12], [1,4,5,6,12,29,109,541,2730,14375,83003])
tests['alquerque_lud'] = (100000,197.12,[52.77,47.23], [1,4,8,33,210,1430,10262,81306,717196,6784234,69173829])
tests['amazons'] = (200,71.46,[50.02,49.98], [1,2176])#,4307152
tests['amazons_split2'] = (1000,136.33,[50.11,49.89], [1,80,2176,168420,4307152])#,4307152
tests['ataxx'] = (10000,115.43,[50.00,50.00], [1,16,256,6424,156520])#,4975152
tests['backgammon'] = (1000,109.98,[48.00,52.00], [1,36,2574,92664,6545432])#,235635552
tests['battleships'] = (10000,195.53,[45.47,54.53], [1,120,14400,1850736])
tests['bombardment'] = (20000,22.74,[51.31,48.69], [1,38,1444,48564,1633284])
tests['breakthrough'] = (20000,64.10,[50.92,49.08], [1,22,484,11132,256036,6182818])#,149264638
tests['chess'] = (1000,408.73,[50.02,49.98], [1,20,400,8902,197281,4865609,119060324])#,3195901860,84998978956,2439530234167,69352859712417
tests['chess_kingCapture'] = (1000,117.95,[50.02,49.98], [1,20,400,8902,197742,4897256,120909363])#,3283514875
tests['chessCylinder'] = (1000,399.82,[50.07,49.93], [1,20,392,9162,211036,5637296,149227488])#,4433920826
tests['chessCylinder_kingCapture'] = (1000,113.22,[50.15,49.85], [1,20,400,9646,231440,6459255,179301818])#,5591796460
tests['chessGardner5x5_kingCapture'] = (10000,35.82,[49.57,50.43], [1,7,53,521,5203,62814,763580,10578142,147664616])
tests['chessLosAlamos6x6_kingCapture'] = (10000,53.59,[50.06,49.94], [1,10,100,1216,14914,208461,2938196,45639750])#,715681456
tests['chessQuick5x6_kingCapture'] = (10000,48.23,[49.89,50.11], [1,6,36,316,2817,30779,340993,4308630,55103454])
tests['chessSilverman4x5_kingCapture'] = (100000,25.13,[49.14,50.86], [1,4,18,121,838,7722,71967,776786,8436486])
tests['clobber'] = (10000,64.63,[48.88,51.12], [1,180,31252,5231000])
tests['connect4'] = (20000,21.31,[55.72,44.28], [1,7,49,343,2401,16807,117649,823536,5673234,39394572])#,268031646
tests['dashGuti'] = (100000,55.84,[61.33,38.66], [1,4,4,6,13,43,100,312,943,3564,14041,57217,254649])
tests['dotsAndBoxes'] = (10000,137.13,[50.00,50.00], [1,144,20592,2924064])
tests['englishDraughts'] = (10000,66.79,[49.29,50.71], [1,7,49,302,1469,7361,36768,179740,845931,3963680,18391564,85242128])#,388623673,1766623630
tests['internationalDraughts'] = (10000,92.17,[49.26,50.74], [1,9,81,658,4265,27117,167140,1049442,6483971])#,41022614,258935682
tests['foxAndGeese'] = (20000,100.86,[49.87,50.13], [1,27,338,1146,18839,79868,1470459])#,6361465
tests['golSkuish'] = (100000,62.44,[51.46,48.54], [1,3,5,7,18,56,146,448,1522,5256,19908,75088,320414])
tests['gomoku_standard'] = (10000,112.59,[50.97,49.03], [1,225,50400,11239200])#,2495102400
tests['gomoku_freeStyle'] = (10000,109.0,[51.03,48.97], [1,225,50400,11239200])#,2495102400
tests['hex'] = (1000,107.52,[52.27,47.73], [1,121,14520,1727880])
tests['hex_9x9'] = (10000,71.02,[53.03,46.97], [1,81,6480,511920])#,39929760
tests['knightthrough'] = (10000,33.64,[51.67,48.33], [1,40,1600,63520,2521306,99598454])#,3929482778
tests['lauKataKati'] = (100000,54.15,[55.48,44.52], [1,3,3,3,7,27,47,181,516,1996,7617,30882,132119])
tests['oware'] = (10000,83.58,[49.40,50.60], [1,6,36,190,1014,5219,27332,139157,711414])#,3592872,18137964
tests['pentago'] = (10000,29.07,[53.57,46.43], [1,288,80640,21934080])
tests['pentago_split'] = (10000,54.74,[54.67,45.33], [1,36,288,10080,80640,2741760])
tests['pretwa'] = (100000,21.00,[52.00,48.00], [1,3,5,7,18,56,146,444,1442,4834,17712])
tests['reversi'] = (1000,60.41,[47.51,52.49], [1,4,12,56,244,1396,8200,55092,390216,3005288,24571056])#,1939879668
tests['skirmish'] = (10000,100.00,[48.89,51.11], [1,20,400,8902,197742,4897256,120921506])#,3284299550
tests['surakarta'] = (10000,239.45,[50.51,49.49], [1,16,256,5382,111122,2572484,58479230])#,1442032302
tests['ticTacDie'] = (100000,7.63,[64.87,35.13], [1,1,9,9,72,72,504,504,3024,3024,15120,13680,54720,49392,148176,100224,200448,127872,127872,0])
tests['ticTacToe'] = (100000,7.63,[64.84,35.16], [1,9,72,504,3024,15120,54720])#,148176,200448,127872
tests['theMillGame'] = (100000,62.58,[52.25,47.75], [1,24,552,12144,255024,5140800,99274176])#,1873562112
tests['twentyOne'] = (1000000,1.88,[0.37], [1,2,52,104,2464,5304,114128,141848,2309136])#,2622336,22543488
tests['ultimateTicTacToe'] = (100000,58.92,[52.42,47.58], [1,81,720,6336,55080,473256,4020960,33782544])

if "all" in games:
  games = []
  
  # Tests
  
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
  games.append('simpleApplyDoubleTest.rg')
  games.append('simpleApplyDoubleRevTest.rg')
  games.append('simpleApplyDoubleSameTest.rg')
  
  games.append('chessTest1.hrg')
  
  # Simple games
  
  games.append('alquerque.py')
  games.append('alquerque.rbg')
  games.append('alquerque_lud.py')
  games.append('alquerque_lud.rbg')

  games.append('bombardment.hrg')

  games.append('breakthrough.rg')
  games.append('breakthrough-simple.rg')
  games.append('breakthrough.hrg')
  games.append('breakthrough.rbg')

  games.append('clobber.hrg')

  games.append('connect4.hrg')
  games.append('connect4.rbg')

  games.append('dashGuti.py')
  games.append('dashGuti.rbg')

  games.append('foxAndGeese.hrg')

  games.append('gomoku_standard.hrg')
  games.append('gomoku_standard.rbg')

  games.append('gomoku_freeStyle.hrg')
  games.append('gomoku_freeStyle.rbg')

  games.append('knightthrough.hrg')
  games.append('knightthrough.rbg')

  games.append('lauKataKati.py')
  games.append('lauKataKati.rbg')
  
  games.append('pretwa.py')
  games.append('pretwa.rbg')
  
  games.append('ticTacDie.hrg')

  games.append('ticTacToe.rg')
  games.append('ticTacToe.hrg')
  games.append('ticTacToe.rbg')
  
  # Complex and large games

  games.append('amazons.hrg')
  games.append('amazons.rbg')
  
  games.append('amazons_split2.hrg')
  games.append('amazons_split2.rbg')
  
  games.append('ataxx.hrg')

  games.append('backgammon.hrg')

  games.append('battleships.hrg')

  games.append('chess.hrg')
  games.append('chess.rbg')

  games.append('chess_kingCapture.rbg')

  games.append('chessCylinder.rbg')

  games.append('chessCylinder_kingCapture.rbg')
  
  games.append('dotsAndBoxes.hrg')

  games.append('englishDraughts.hrg')
  games.append('englishDraughts.rbg')
  
  games.append('hex.rbg')
  games.append('hex_9x9.rbg')

  games.append('golSkuish.py')
  games.append('golSkuish.rbg')
  
  games.append('oware.hrg')

  games.append('pentago.hrg')
  
  games.append('surakarta.rbg')
  
  games.append('theMillGame.rbg')

  games.append('ultimateTicTacToe.hrg')

elif "short" in games:
  games = []
  
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
  games.append('simpleApplyDoubleTest.rg')
  games.append('simpleApplyDoubleRevTest.rg')
  games.append('simpleApplyDoubleSameTest.rg')
  
  games.append('alquerque.py')
  games.append('breakthrough.rbg')

  games.append('chess.hrg')
  games.append('englishDraughts.hrg')

  games.append('pretwa.rbg')


print(f'Testing #{len(games)}: {" ".join(games)}')
print(f'Translate options: {translateOptions}')
print(f'rg2cpp options: {cfg.DEFAULT_RG2CPP_OPTIONS}')
print(f'g++ {infoGccOptions} options: {cppFlags}')

#######################################################################################################################

HEAD_FORMATTER = '{: <50} '
TIME_FORMATTER = '{: <20}{:9.3f} s'
SIMS_FORMATTER = '{: <20}{:9.3f} s  {:15,.0f} states/s  {:15,.0f} sims/s'
PERFT_FORMATTER = '{: <20}{:9.3f} s  {:15,.0f} states/s'

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

  parsed = parseGameName(game)
  if parsed == None: exit(1)
  (gameName,gameFile) = parsed

  if gameName not in tests:
    print(f'No tests for game {gameName} for {game}')
    continue

  ######## Translate and compile ########
  print(HEAD_FORMATTER.format(f'{game} ast:'),end='',flush=True)
  startTime = time.time()
  error = createAST(game, translateOptions, True)
  elapsedTime = time.time() - startTime
  if error != None: info = f'{util.ERROR}\n{util.CYAN}{error}{util.RESET}'
  else: info = f'{util.OK}'
  print(TIME_FORMATTER.format(info,elapsedTime))
  if error != None:
    print(f'{util.CYAN}{error}{util.RESET}')
    continue
      
  print(HEAD_FORMATTER.format(f'{game} rg2cpp:'),end='',flush=True)
  startTime = time.time()
  error = rg2cpp(game, cfg.DEFAULT_RG2CPP_OPTIONS + ' ' + cfg.DEBUG_RG2CPP_OPTIONS, True)
  elapsedTime = time.time() - startTime
  if error != None: info = f'{util.ERROR}\n{util.CYAN}{error}{util.RESET}'
  else: info = f'{util.OK}'
  print(TIME_FORMATTER.format(info,elapsedTime))
  if error != None:
    print(f'{util.CYAN}{error}{util.RESET}')
    continue
  
  isOK = True
  
  ######## Sims ########
  print(HEAD_FORMATTER.format(f'{game} {compiler} sims:'),end='',flush=True)
  startTime = time.time()
  error = compileCpp('sims', compiler, cppFlags)
  elapsedTime = time.time() - startTime
  if error != None: info = f'{util.ERROR}\n{util.CYAN}{error}{util.RESET}'
  else: info = f'{util.OK}'
  print(TIME_FORMATTER.format(info,elapsedTime))
  if error != None:
    print(f'{util.CYAN}{error}{util.RESET}')
    continue

  sims = tests[gameName][0]
  avgDepth = tests[gameName][1]
  avgScores = tests[gameName][2]
  print(HEAD_FORMATTER.format(f'{game} sims {sims:}:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {sims}')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    info = f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}'
    isOK = False
    errInfo = decodeOutput(result.stderr).strip()
    resSims = 0
    resStates = 0
  else:
    expectedList = [avgDepth] + avgScores
    stats = decodeOutput(result.stdout).strip().split(' ')
    resSims = int(stats[1])
    resStates = int(stats[2])
    resultList = [resStates / sims] # avgDepth
    stats = stats[8:]
    for p in range(len(stats)//3): resultList.append(int(stats[p*3]) / sims) # avgScores
    if not verifyWithTolerance(expectedList, resultList):
      info = f'{util.ERROR} expected {util.CYAN}{" ".join(f"{x:1.2f}" for x in expectedList)}{util.RESET} but got {util.CYAN}{" ".join(f"{x:1.2f}" for x in resultList)}{util.RESET}'
      isOK = False
    else:
      info = f'{util.OK}'
    errInfo = None
  print(SIMS_FORMATTER.format(info, elapsedTime, resStates/elapsedTime, resSims/elapsedTime).replace(',',' '))
  if errInfo != None: print(f'{util.CYAN}{errInfo}{util.RESET}')

  ######## Perft ########
  print(HEAD_FORMATTER.format(f'{game} {compiler} perft:'),end='',flush=True)
  startTime = time.time()
  error = compileCpp('perft', compiler, cppFlags)
  elapsedTime = time.time() - startTime
  if error != None: info = f'{util.ERROR}\n{util.CYAN}{error}{util.RESET}'
  else: info = f'{util.OK}'
  print(TIME_FORMATTER.format(info,elapsedTime))
  if error != None:
    print(f'{util.CYAN}{error}{util.RESET}')
    continue
    
  expectedPerft = tests[gameName][3]
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
        isOK = False
      else:
        info = f'{util.OK}'
      errInfo = None
    print(PERFT_FORMATTER.format(info, elapsedTime, resStates/elapsedTime))
    if errInfo != None: print(f'{util.CYAN}{errInfo}{util.RESET}')

  if isOK: gamesOK.append(game)
  ################

totalElapsedTime = time.time() - totalStartTime
gamesBad = [game for game in games if game not in gamesOK]

print()
print((HEAD_FORMATTER+TIME_FORMATTER).format(f'--- Summary --- {util.GREEN}{util.RESET}', '', totalElapsedTime))
print(f'Games {util.OK}: {" ".join(gamesOK)}')
if len(gamesBad) == 0:
  print(f'No errors.')
else:
  print(f'Games with {util.ERROR}: {" ".join(gamesBad)}')
