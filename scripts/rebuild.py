#!/usr/bin/env python3
import sys, os, time
from common import *
os.chdir(os.path.dirname(sys.argv[0])+"/..") # RGCompiler dir

startTime = time.time()
run(f'rm -rf {cfg.BUILD_DIR}')
run(f'mkdir -p {cfg.BUILD_DIR}')
run(f'mkdir -p {cfg.BUILD_TEST_DIR}')
os.chdir(cfg.BUILD_DIR)
run(f'cmake .. && make')
elapsedTime = time.time() - startTime
print(f'Built time {elapsedTime:9.3f} s')
