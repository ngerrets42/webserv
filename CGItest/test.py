#!/usr/local/bin/python3

import os
import sys

print(os.getenv('CONTENT_LENGTH'))
data = input();
#data = sys.stdin.buffer.read(10)

print(data.upper());
