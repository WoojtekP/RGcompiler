#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Benchmark simulations.')
parser.add_argument('game', nargs='+', help='run tests for these games (use \"all\" for all default predefined tests')
parser.add_argument('limit', nargs=1, help='number of simulations (int) or time in seconds (float, ended with "s")')
parser.add_argument('-t', dest='translateFlags', nargs='?', help='translate flags for interpreter_rust', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('--reuse', action='store_true', help='reuse already built reasoner (no translation nor rg2cpp)')
parser.add_argument('--perf', action='store_true', help='count instructions by perf')
parser.add_argument('--clang', action='store_true', help='use clang++ instead of g++')
cpp_flags_group = parser.add_mutually_exclusive_group(required=False)
cpp_flags_group.add_argument('--benchmark', action='store_true', help='maximum speed flags (default)')
cpp_flags_group.add_argument('--test', action='store_true', help='optimization and asserts')
cpp_flags_group.add_argument('--debug', action='store_true', help='gdb symbols and sanitizers')
cpp_flags_group.add_argument('--profile', action='store_true', help='generate profiler information')

args = parser.parse_args()
games = args.game
if args.limit[0].endswith('s'):
  useTime = 1
  limitS = float(args.limit[0][:-1])
  limit = int(round(limitS * 1000.0))
else:
  useTime = 0
  limit = int(args.limit[0])
translateFlags = args.translateFlags
benchmarkMode = False
if args.profile:
  cppFlags = cfg.GCC_PROFILE_FLAGS
  mode = "profile"
elif args.debug:
  cppFlags = cfg.GCC_DEBUG_FLAGS
  mode = "debug"
elif args.test:
  cppFlags = cfg.GCC_TEST_FLAGS
  mode = "test"
else:
  cppFlags = cfg.GCC_BENCHMARK_FLAGS
  mode = "benchmark"
  benchmarkMode = True

if args.clang: compiler = 'clang++'
else: compiler = 'g++'

if "all" in games:
  games = []
  games.append('alquerque.py')
  games.append('alquerque_lud.py')
  games.append('amazons.hrg')
  games.append('amazons_split2.hrg')
  games.append('ataxx.hrg')
  games.append('backgammon.hrg')
  #games.append('battleships.hrg')
  games.append('bombardment.hrg')
  games.append('breakthrough.hrg')
  games.append('chess.hrg')
  games.append('chess_kingCapture.hrg')
  games.append('clobber.hrg')
  games.append('connect4.hrg')
  games.append('dashGuti.py')
  games.append('dashGuti_lud.py')
  games.append('dotsAndBoxes.hrg')
  games.append('englishDraughts.hrg')
  games.append('foxAndGeese.hrg')
  games.append('golSkuish.py')
  games.append('golSkuish_lud.py')
  games.append('gomoku_standard.hrg')
  games.append('knightthrough.hrg')
  games.append('lauKataKati.py')
  games.append('lauKataKati_lud.py')
  games.append('oware.hrg')
  games.append('pentago.hrg')
  games.append('pentago_split.hrg')
  games.append('pretwa.py')
  games.append('pretwa_lud.py')
  games.append('ticTacDie.hrg')
  #games.append('twentyOne.hrg')
  games.append('ultimateTicTacToe.hrg')
  
  games.append('alquerque.rbg')
  games.append('alquerque_lud.rbg')
  games.append('amazons.rbg')
  games.append('amazons_split2.rbg')
  games.append('breakthrough.rbg')
  # games.append('chessGardner5x5_kingCapture.rbg')
  # games.append('chessLosAlamos6x6_kingCapture.rbg')
  # games.append('chessQuick5x6_kingCapture.rbg')
  # games.append('chessSilverman4x5_kingCapture.rbg')
  games.append('chess.rbg')
  games.append('chess_kingCapture.rbg')
  games.append('connect4.rbg')
  games.append('dashGuti.rbg')
  games.append('englishDraughts.rbg')
  games.append('englishDraughts_lud.rbg')
  games.append('foxAndHounds.rbg')
  games.append('golSkuish.rbg')
  games.append('gomoku_standard.rbg')
  games.append('hex.rbg')
  games.append('knightthrough.rbg')
  games.append('lauKataKati.rbg')
  # games.append('pentago.rbg')
  # games.append('pentago_split.rbg')
  games.append('pretwa.rbg')
  games.append('reversi.rbg')
  # games.append('skirmish.rbg')
  games.append('theMillGame.rbg')
  games.append('theMillGame_lud.rbg')
  games.append('yavalath.rbg')
  
  # games.append('breakthrough.kif')
  # games.append('connect4.kif')
  # games.append('hex.kif')
  # games.append('knightthrough.kif')

elif "short" in games:
  games = []
  games.append('alquerque.hrg')
  games.append('breakthrough.hrg')
  games.append('chess.hrg')
  games.append('chessCylinder.rbg')
  games.append('connect4.hrg')
  games.append('dotsAndBoxes.hrg')
  games.append('englishDraughts.hrg')
  games.append('pentago.hrg')
  games.append('pretwa.rbg')
  games.append('yavalath.rbg')


print(f'Mode {util.GREEN}{mode}{util.RESET}, limit: ',end='')
if useTime: print(f'{limitS}s')
else: print(f'{limit} sims')
print(f'{len(games)} games: {" ".join(games)}')
print(f'Translate flags: {translateFlags}')
print(f'rg2cpp flags: {cfg.DEFAULT_RG2CPP_OPTIONS}')
print(f'{compiler} flags: {cppFlags}')
print()

HEAD_FORMATTER = '{: <50} '
TIME_FORMATTER = '{:6.3f}s'
STATES_STAT_FORMATTER = ' {:15,.0f} states/s'
STATES_STAT_PRECISE_FORMATTER = ' {:11,.3f} states/s'
SIMS_STAT_FORMATTER = ' {:15,.0f} sims/s'
SIMS_STAT_PRECISE_FORMATTER = ' {:11,.3f} sims/s'
INSTR_SCALE = 1_000
INSTR_FORMATTER = ' {:12,.0f} k instr'

sumASTTime = 0
sumRg2CppTime = 0
sumCppTime = 0
sumSimsTime = 0
sumSimsCount = 0
if args.perf: sumInstr = 0
gamesOK = []

#######################################################################################################################
for game in games:
  print(HEAD_FORMATTER.format(f'{game}:'),end='',flush=True)
  
  ######## Translate ########
  if not args.reuse:
    print(f' | ast ',end='',flush=True)
    startTime = time.time()
    error = createAST(game, translateFlags, True)
    if error != None:
      print(f'\n{util.ERROR} {util.CYAN}{error}{util.RESET}')
      continue
    elapsedTime = time.time() - startTime
    print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)
    sumASTTime += elapsedTime
    
    print(f' | rg2cpp ',end='',flush=True)
    startTime = time.time()
    result = rg2cpp(game, cfg.DEFAULT_RG2CPP_OPTIONS + ' ' + cfg.DEBUG_RG2CPP_OPTIONS, True)
    if error != None:
      print(f'\n{util.ERROR} {util.CYAN}{error}{util.RESET}')
      continue
    elapsedTime = time.time() - startTime
    print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)
    sumRg2CppTime += elapsedTime
  
  ######## Compile cpp ########
  print(f' | {compiler} ',end='',flush=True)
  startTime = time.time()
  error = compileCpp('sims', compiler, f'{cppFlags} -DUSE_TIME={useTime}')
  elapsedTime = time.time() - startTime
  if error != None:
    print(f'{util.ERROR} {util.CYAN}{error}{util.RESET}')
    continue
  print(TIME_FORMATTER.format(elapsedTime),end='',flush=True)
  sumCppTime += elapsedTime
  
  ######## Sims ########
  print(f' | sims ',end='',flush=True)
  if args.perf:
    result = runCap(f'perf stat -e instructions -x " " {cfg.BUILD_TEST_DIR}/sims {limit}')
  else:
    result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {limit}')

  if result.returncode != 0:
    print(f'{util.ERROR} exitcode {result.returncode}')
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
  else:
    stats = decodeOutput(result.stdout).strip().split(' ')
    elapsedTime = int(stats[0]) * 0.001 # Read time in ms
    resSims = int(stats[1])
    resStates = int(stats[2])

    if args.perf:
      output = decodeOutput(result.stderr)
      if str.isnumeric(output.split(' ')[0]):
        elapsedInstr = int(output.split(' ')[0]) / INSTR_SCALE
        sumInstr += elapsedInstr
        print((TIME_FORMATTER+INSTR_FORMATTER+STATES_STAT_FORMATTER+SIMS_STAT_FORMATTER).format(elapsedTime, elapsedInstr, resStates/elapsedTime, resSims/elapsedTime).replace(',',' '))
      else:
        print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
        print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
        continue
    else:
      print(TIME_FORMATTER.format(elapsedTime),end='')
      formatter = STATES_STAT_PRECISE_FORMATTER if resStates < 10 else STATES_STAT_FORMATTER
      print(formatter.format(resStates/elapsedTime).replace(',',' '),end='')
      formatter = SIMS_STAT_PRECISE_FORMATTER if resSims < 10 else SIMS_STAT_FORMATTER
      print(formatter.format(resSims/elapsedTime).replace(',',' '))
    
    if not benchmarkMode:
      resAvgDepth = resStates / resSims
      resMinDepth = int(stats[3])
      resMaxDepth = int(stats[4])
      resMoves = int(stats[5])
      resMinMoves = int(stats[6])
      resMaxMoves = int(stats[7])
      stats = stats[8:]
      resAvgScores = []
      resMinScores = []
      resMaxScores = []
      for p in range(len(stats)//3):
        resAvgScores.append(int(stats[p*3]) / resSims)
        resMinScores.append(int(stats[p*3+1]))
        resMaxScores.append(int(stats[p*3+2]))
      print(f'sims: {resSims} states: {resStates}')
      print(f'depth: min {resMinDepth} avg {resAvgDepth:1.2f} max {resMaxDepth}')
      print(f'moves: min {resMinMoves} avg {resMoves/resStates:1.2f} max {resMaxMoves}')
      print(f'avg scores: {" ".join(f"{avgScore:1.2f}" for avgScore in resAvgScores)}  min scores: {" ".join(f"{minScore}" for minScore in resMinScores)}  max scores: {" ".join(f"{maxScore}" for maxScore in resMaxScores)}')
      print()
    
    sumSimsTime += elapsedTime
    sumSimsCount += resSims
    
    if args.profile: run(f'gprof {cfg.BUILD_TEST_DIR}/sims gmon.out > gmon.txt')

#######################################################################################################################
if len(games) > 1:
  print()
  print(f'--- Summary ---')
  FORMATTER = HEAD_FORMATTER + '{:.3f}s'
  print(FORMATTER.format(f'Total AST time:', sumASTTime))
  print(FORMATTER.format(f'Total rg2cpp time:', sumRg2CppTime))
  print(FORMATTER.format(f'Total {compiler} time:', sumCppTime))
  print(FORMATTER.format(f'Total sims time:', sumSimsTime))
  if args.perf: print((HEAD_FORMATTER+'{:,.0f} k').format(f'Total instructions:', sumInstr))
  print((HEAD_FORMATTER+'{:,.0f}').format(f'Total sims count:', sumSimsCount))
