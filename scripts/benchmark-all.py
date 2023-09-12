#!/usr/bin/env python3
import sys, os, argparse, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

parser = argparse.ArgumentParser(description='Benchmark with predefined tests.')
parser.add_argument('games', nargs='*', help='run tests only for these games')
parser.add_argument('-t', dest='translateOptions', nargs='?', help='translate options for interpreter_node/lib/cli', default=cfg.DEFAULT_TRANSLATE_OPTIONS)
parser.add_argument('-noperf', action='store_true', help='disable using perf for counting instructions')
parser.add_argument('-skipcompilation', action='store_true', help='skip compile.py and use the existing reasoner sources')

args = parser.parse_args()
games = args.games
translateOptions = '"' + args.translateOptions + '"'

usePerf = False
if not args.noperf:
  if not isProgramAvailable('perf'):
    print('WARN Measuring only runtime: perf command unavailable')
  else:
    paranoid = runCap('cat /proc/sys/kernel/perf_event_paranoid')
    if paranoid.returncode != 0:
      print('WARN Measuring only runtime: cannot read perf_event_paranoid -- you have a wrong OS')
    else:
      paranoid = int(paranoid.stdout.decode('UTF-8'))
      if paranoid >= 4:
        print(f'WARN Measuring only runtime: perf_event_paranoid = {paranoid} (fix with "echo 3 | sudo tee /proc/sys/kernel/perf_event_paranoid")')
      else:
        usePerf = True

tests = {}
tests['breakthrough.rg'] =  (100_000, 5)
tests['breakthrough.hrg'] = (100_000, 5)
tests['breakthrough.rbg'] = (100_000, 5)
tests['knightthrough.hrg'] = (100_000, 5)
tests['connect4.hrg'] =     (200_000, 8)
tests['amazons-smart.hrg'] =  (1000, 1)

if len(games) == 0:
  games.append('breakthrough.rg')
  games.append('breakthrough.hrg')
  games.append('breakthrough.rbg')
  games.append('connect4.hrg')
  games.append('knightthrough.hrg')
  games.append('amazons-smart.hrg')

print(f'Testing: {" ".join(games)}')
print(f'with translate options {translateOptions}')
print(f'Using perf: {usePerf}')

HEAD_FORMATTER = '{: <50} '
TIME_FORMATTER = '{:9.3f} s'
INSTR_FORMATTER = ' {:9,.0f} mil instr'

INSTR_SCALE = 1_000_000

sumCompileTime = 0
sumGCCTime = 0
sumSimsTime = 0
sumSimsInstr = 0
sumPerftTime = 0
sumPerftInstr = 0
gamesOK = []

for game in games:
  print()
  
  if not args.skipcompilation:
    print(HEAD_FORMATTER.format(f'{game} compile:'),end='',flush=True)
    startTime = time.time()
    result = runCap(f'python3 scripts/compile.py {game} -t{translateOptions}')
    elapsedTime = time.time() - startTime
    if result.returncode != 0:
      print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
      print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
      continue
    else:
      print(TIME_FORMATTER.format(elapsedTime))
      sumCompileTime += elapsedTime
  
  print(HEAD_FORMATTER.format(f'{game} g++:'),end='',flush=True)
  startTime = time.time()
  result = runCap(f'''
    g++ -c {cfg.BUILD_TEST_DIR}/reasoner.cpp -I{cfg.BUILD_TEST_DIR} {cfg.GCC_BENCHMARK_FLAGS} -o {cfg.BUILD_TEST_DIR}/reasoner.o &&
    g++ test/sims.cpp {cfg.BUILD_TEST_DIR}/reasoner.o -I{cfg.BUILD_TEST_DIR} {cfg.GCC_BENCHMARK_FLAGS} -o {cfg.BUILD_TEST_DIR}/sims &&
    g++ test/perft.cpp {cfg.BUILD_TEST_DIR}/reasoner.o -I{cfg.BUILD_TEST_DIR} {cfg.GCC_BENCHMARK_FLAGS} -o {cfg.BUILD_TEST_DIR}/perft
  ''')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
    continue
  else:
    print(TIME_FORMATTER.format(elapsedTime))
    sumGCCTime += elapsedTime
  
  sims = tests[game][0]
  print(HEAD_FORMATTER.format(f'{game} sims {sims}:'),end='',flush=True)
  startTime = time.time()
  if usePerf:
    result = runCap(f'perf stat -e instructions -x " " {cfg.BUILD_TEST_DIR}/sims {sims}')
  else:
    result = runCap(f'{cfg.BUILD_TEST_DIR}/sims {sims}')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    print(f'{util.ERROR} exitcode {result.returncode}')
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
  else:
    if usePerf:
      output = decodeOutput(result.stderr)
      if str.isnumeric(output.split(' ')[0]):
        elapsedInstr = int(output.split(' ')[0]) / INSTR_SCALE
        print((TIME_FORMATTER+INSTR_FORMATTER).format(elapsedTime, elapsedInstr))
        sumSimsTime += elapsedTime
        sumSimsInstr += elapsedInstr
      else:
        print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
        print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
    else:
      print(TIME_FORMATTER.format(elapsedTime))
      sumSimsTime += elapsedTime

  depth = tests[game][1]
  print(HEAD_FORMATTER.format(f'{game} perft {depth}:'),end='',flush=True)
  startTime = time.time()
  if usePerf:
    result = runCap(f'perf stat -e instructions -x " " {cfg.BUILD_TEST_DIR}/perft {depth}')
  else:
    result = runCap(f'{cfg.BUILD_TEST_DIR}/perft {depth}')
  elapsedTime = time.time() - startTime
  if result.returncode != 0:
    print(f'{util.ERROR} {util.CYAN}exitcode {result.returncode}{util.RESET}')
    print(f'{util.CYAN}{decodeOutput(result.stderr).strip()}{util.RESET}')
  else:
    if usePerf:
      output = decodeOutput(result.stderr).strip()
      if str.isnumeric(output.split(' ')[0]):
        elapsedInstr = int(output.split(' ')[0]) / INSTR_SCALE
        print((TIME_FORMATTER+INSTR_FORMATTER).format(elapsedTime, elapsedInstr))
        sumPerftTime += elapsedTime
        sumPerftInstr += elapsedInstr
      else:
        print(f'{util.ERROR} {util.CYAN}(exitcode {result.returncode}) {output}{util.RESET}')
        print(f'{util.CYAN}{output}{util.RESET}')
    else:
      print(TIME_FORMATTER.format(elapsedTime))
      sumPerftTime += elapsedTime


if len(games) > 1:
  print()
  print(f'--- Summary ---')
  FORMATTER = HEAD_FORMATTER + TIME_FORMATTER
  print(FORMATTER.format(f'Total compile:', sumCompileTime))
  print(FORMATTER.format(f'Total g++:', sumGCCTime))
  if usePerf:
    FORMATTER += INSTR_FORMATTER
    print(FORMATTER.format(f'Total sims:', sumSimsTime, sumSimsInstr))
    print(FORMATTER.format(f'Total perft:', sumPerftTime, sumPerftInstr))
  else:
    print(FORMATTER.format(f'Total sims:', sumSimsTime))
    print(FORMATTER.format(f'Total perft:', sumPerftTime))
