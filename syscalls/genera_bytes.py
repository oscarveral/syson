#! /usr/bin/env python3

import random
import argparse
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--semilla", help="semilla para el generador de números aleatorios", type=int, default=42)
parser.add_argument("-n", "--numbytes", help="número de bytes aleatorios generados", type=int, required=True)
parser.add_argument("-t", "--tamwrites", help="número de bytes a escribir cada vez", type=int, default=int(16*1024*1024))
args = parser.parse_args()

random.seed(args.semilla)

for i in range(args.numbytes//args.tamwrites):
    os.write(sys.stdout.fileno(), random.randbytes(args.tamwrites))
os.write(sys.stdout.fileno(), random.randbytes(args.numbytes % args.tamwrites))
