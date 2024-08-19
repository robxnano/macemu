#!/usr/bin/env python3

import os,subprocess,sys

try:
    os.chdir(sys.argv[2])
    subprocess.run(sys.argv[1])
except:
    exit(1)
