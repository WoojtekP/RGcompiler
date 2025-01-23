#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Benchmark simulations.')
parser.add_argument('game', nargs='+', help='run tests for these games (use \"all\" for all default predefined tests')
parser.add_argument('limit', nargs=1, help='number of simulations (int) or time in seconds (float, ended with "s")')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-skipcompilation', action='store_true', help='skip compile.py and use the existing reasoner sources')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('-benchmark', action='store_true', help='maximum speed flags (default)')
cpp_flags_group.add_argument('-test', action='store_true', help='optimization with asserts')
cpp_flags_group.add_argument('-debug', action='store_true', help='compile with gdb symbols')
cpp_flags_group.add_argument('-profile', action='store_true', help='generate profiler information')

args = parser.parse_args()
games = args.game
if args.limit[0].endswith('s'):
  useTime = 1
  limitS = float(args.limit[0][:-1])
  limit = int(round(limitS * 1000.0))
else:
  useTime = 0
  limit = int(args.limit[0])
translateOptions = args.translateOptions
if args.profile:
  gccOptions = cfg.GCC_PROFILE_FLAGS
  infoGccOptions = "profile"
elif args.debug:
  gccOptions = cfg.GCC_DEBUG_FLAGS
  infoGccOptions = "debug"
elif args.test:
  gccOptions = cfg.GCC_TEST_FLAGS
  infoGccOptions = "test"
else:
  gccOptions = cfg.GCC_BENCHMARK_FLAGS
  infoGccOptions = "benchmark"
  
if "all" in games:
  games = []
  games.append('amazons.hrg')
  games.append('amazons_split2.hrg')
  games.append('ataxx.hrg')
  games.append('battleships.hrg')
  games.append('bombardment.hrg')
  games.append('backgammon-opt.hrg')# TODO
  games.append('breakthrough.hrg')
  games.append('chess.hrg')
  games.append('clobber.hrg')
  games.append('connect4.hrg')
  games.append('dotsAndBoxes.hrg')
  games.append('englishDraughts-extratags.hrg')# TODO
  games.append('foxAndGeese.hrg')
  games.append('gomoku_standard.hrg')
  games.append('knightthrough.hrg')
  games.append('oware.hrg')
  games.append('pentago.hrg')
  games.append('pentago_split.hrg')
  games.append('ticTacDie.hrg')
  games.append('twentyOne.hrg')
  
  games.append('amazons.rbg')
  games.append('amazons_split2.rbg')
  games.append('breakthrough.rbg')
  # games.append('chessGardner5x5_kingCapture.rbg')
  # games.append('chess_kingCapture_200.rbg')
  # games.append('chess_kingCapture.rbg')
  # games.append('chessLosAlamos6x6_kingCapture.rbg')
  # games.append('chessQuick5x6_kingCapture.rbg')
  games.append('chess.rbg')
  # games.append('chessSilverman4x5_kingCapture.rbg')
  games.append('connect4.rbg')
  # games.append('connect6_split.rbg')
  games.append('englishDraughts.rbg')
  # games.append('englishDraughts_split.rbg')
  # games.append('foxAndHounds.rbg')
  # games.append('go_constsum.rbg')
  # games.append('gomoku_freeStyle.rbg')
  games.append('gomoku_standard.rbg')
  # games.append('go_nopass.rbg')
  # games.append('go.rbg')
  games.append('hex.rbg')
  # games.append('internationalDraughts.rbg')
  games.append('knightthrough.rbg')
  # games.append('knightthrough_split.rbg')
  # games.append('paperSoccer.rbg')
  games.append('pentago.rbg')
  games.append('pentago_split.rbg')
  games.append('reversi.rbg')
  games.append('skirmish.rbg')
  games.append('theMillGame.rbg')
  # games.append('theMillGame_split.rbg')
  # games.append('ticTacToe.rbg')
  games.append('yavalath.rbg')

print(f'Testing #{len(games)}: {" ".join(games)}')
print(f'Translate options: {translateOptions}')
print(f'rg2cpp options: {cfg.DEFAULT_RG2CPP_OPTIONS}')
print(f'g++ {infoGccOptions} options: {gccOptions}')
if useTime: print(f'Limit: {limitS}s')
else: print(f'Limit: {limit} sims')
print()

HEAD_FORMATTER = '{: <50} '
TIME_FORMATTER = '{:7.3f}s'
STATES_STAT_FORMATTER = ' {:15,.0f} states/s'
STATES_STAT_PRECISE_FORMATTER = ' {:11,.3f} states/s'
SIMS_STAT_FORMATTER = ' {:15,.0f} sims/s'
SIMS_STAT_PRECISE_FORMATTER = ' {:11,.3f} sims/s'
INSTR_FORMATTER = ' {:9,.0f} mil instr'

sumCompileTime = 0
sumGCCTime = 0
sumSimsTime = 0
sumSimsCount = 0
gamesOK = []

#######################################################################################################################
for game in games:
  print(HEAD_FORMATTER.format(f'{game}:'),end='',flush=True)
  
  ######## Compile ########
  if not args.skipcompilation:
    print(f' | compile ',end='',flush=True)
    startTime = time.time()
    result = runCap(f'python3 scripts/compile.py {game} -t"{translateOptions}"')
    elapsedTime = time.time() - startTime
    if result.returncode != 0:
      print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
      print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
      continue
    else:
      print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)
      sumCompileTime += elapsedTime
  
  print(f' | g++ ',end='',flush=True)
  startTime = time.time()
  result = runCap(f'''
    g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {cfg.GCC_BENCHMARK_FLAGS} -DUSE_TIME={useTime} -o {cfg.BUILD_TEST_DIR}/sims
  ''')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
    continue
  else:
    print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)
    sumGCCTime += elapsedTime
  
  ######## Sims ########
  print(f' | sims ',end='',flush=True)
  startTime = time.time()
  result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {limit}')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    print(f'{util.ERROR} exitcode {result.returncode}')
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
  else:
    resOut = decodeOutput(result.stdout).strip().split(' ')
    resSims = int(resOut[0])
    resStates = int(resOut[1])
    
    print(TIME_FORMATTER.format(elapsedTime),end='')
    formatter = STATES_STAT_PRECISE_FORMATTER if resStates < 10 else STATES_STAT_FORMATTER
    print(formatter.format(resStates/elapsedTime).replace(',',' '),end='')
    formatter = SIMS_STAT_PRECISE_FORMATTER if resSims < 10 else SIMS_STAT_FORMATTER
    print(formatter.format(resSims/elapsedTime).replace(',',' '))
    
    sumSimsTime += elapsedTime
    sumSimsCount += resSims

#######################################################################################################################
if len(games) > 1:
  print()
  print(f'--- Summary ---')
  FORMATTER = HEAD_FORMATTER + TIME_FORMATTER
  print(FORMATTER.format(f'Total compile:', sumCompileTime))
  print(FORMATTER.format(f'Total g++:', sumGCCTime))
  print(FORMATTER.format(f'Total sims time:', sumSimsTime))
  print(FORMATTER.format(f'Total sims count:', sumSimsCount))
