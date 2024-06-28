# Copyright (C) 2024, Sorbonne Universite, LIP6
# This file is part of the Blind-Folded work, under the GPL v3.0 license
# See https://www.gnu.org/licenses/gpl-3.0.en.html for license information
# SPDX-License-Identifier: GPL-3.0-only
# Author(s): Xunyue HU

import sys
import os
import subprocess
import numpy as np
import string
from Levenshtein import distance


exp_file_suffix = 'exp'
pattern_file_0 = 'pattern_0.npy'
pattern_file_1 = 'pattern_1.npy'


def usage(argv):
    print('Usage: %s <options> <trace_file>.npy ... <trace_file>.npy' % argv[0])
    print('   <trace_file>.npy: trace file of the traditional (non-windowed) exponentiation in libgcrypt in numpy format')
    print('                     For each given trace file, a file named <trace_file>_exp.txt must contain the exponent for verification')
    print('Options:')
    print('   [-p | --pattern-files]  <file_0.npy> <file_1.npy>: Pattern files corresponding to the mask computation respectively for values 0 and 1 (default: \'pattern_0.npy\' and \'pattern_1.npy\')')


def change_forme_inv_str(d):
    s = ''
    for i in range(len(d)):
        s += format(int("0x" + d[i], 0), '04b')
    #print(s)
    return s


def load_exponent(exp_file):
    file = open(exp_file, 'r')
    lines = file.readlines()
    newlines = []
    for line in lines:
        newline = line
        newline = newline.replace("\n", '')
        newline = newline.replace(' ', '')
        newlines.append(newline)
    content = ''.join(newlines)
    return content



trace_file_list = []
idx = 1
while idx < len(sys.argv):
    if sys.argv[idx] == '-p' or sys.argv[idx] == '--pattern-files':
        idx += 1
        if idx < len(sys.argv):
            pattern_file_0 = sys.argv[idx]
            idx += 1
            if idx < len(sys.argv):
                pattern_file_1 = sys.argv[idx]
    else:
        if os.path.isfile(sys.argv[idx]):
            trace_file_list.append(sys.argv[idx])
        else:
            print('*** Error: file %s does not exist' % sys.argv[idx])
            usage(sys.argv)
            sys.exit(1)
    idx += 1

if len(trace_file_list) == 0:
    print('*** Error: at least one trace file must be provided')
    usage(sys.argv)
    sys.exit(1)

if not os.path.isfile(pattern_file_0):
    print('*** Error: pattern file %s does not exist' % pattern_file_0)
    usage(sys.argv)
    sys.exit(1)

if not os.path.isfile(pattern_file_1):
    print('*** Error: pattern file %s does not exist' % pattern_file_1)
    usage(sys.argv)
    sys.exit(1)

print('# Pattern 0 file: %s' % pattern_file_0)
print('# Pattern 1 file: %s' % pattern_file_1)

# Load generic pattern from python file if exists
pre0, ext0 = os.path.splitext(pattern_file_0)
pre1, ext1 = os.path.splitext(pattern_file_1)
raw_pattern_file_0 = pre0 + '.raw'
raw_pattern_file_1 = pre1 + '.raw'
pattern_0 = np.load(pattern_file_0)
pattern_1 = np.load(pattern_file_1)
if len(pattern_0) != len(pattern_1):
    print('*** Error: patterns in files %s and %s do not have the same size' % (pattern_0_file, pattern_1_file))
    sys.exit(1)
pattern_0_array = np.asarray(pattern_0)
pattern_1_array = np.asarray(pattern_1)
pattern_0_array.astype('float32').tofile(raw_pattern_file_0)
pattern_1_array.astype('float32').tofile(raw_pattern_file_1)

print('# Generated raw pattern 0 file: %s' % raw_pattern_file_0)
print('# Generated raw pattern 1 file: %s' % raw_pattern_file_1)

print('# Using automatic threshold')


cmd = ['gcc', '-O3', 'attack_rsa_traditional.c', '-o', 'attack_rsa_traditional']
print('# Running command: %s' % ' '.join(map(lambda x: '%s' % x, cmd)))
ret = subprocess.run(cmd)

c = 0
b = 0
for trace_file in trace_file_list:
    print("# Processing trace file: %s" % trace_file)
    pre, ext = os.path.splitext(trace_file)
    raw_trace_file = pre + '.raw'

    if not os.path.isfile(raw_trace_file):
        traces = np.load(trace_file)
        trace_array = np.asarray(traces)
        trace_array.astype('float32').tofile(raw_trace_file)
        print('# Generated raw trace file: %s' % raw_trace_file)
    else:
        print('# Raw trace file %s already exists' % raw_trace_file)

    # Get associated exp file
    exp_file = pre + '_' + exp_file_suffix + '.txt'
    print("# Associated exponent file is: %s" % exp_file)
    exponent = load_exponent(exp_file)
    print('# Loaded exponent (key):')
    print(exponent)

    cmd = ['./attack_rsa_traditional', '-t', raw_trace_file, '-p', raw_pattern_file_0, raw_pattern_file_1]
    print('# Running command: %s' % ' '.join(map(lambda x: '%s' % x, cmd)))
    ret = subprocess.run(cmd, capture_output = True, text = True)
    #print(ret.stdout + '\n')

    key_found = (ret.stdout.split("key found:       0b"))[1][:-1].zfill(2048)
    print('# Key found:')
    print('%s' % key_found)
    key = change_forme_inv_str(exponent)

    if key == key_found:
        print("# Found all bits, correct key!")
        c += 1
    else:
        k = key[key.index('1') + 1:]
        kf = key_found[key_found.index('1') + 1:]
        dist = distance(k, kf)
        print("# %d bits are incorrect or missing" % dist)
        b += dist


num_traces = len(trace_file_list)
print("# Success rate at the trace level: %.2f%% (%d / %d)" % ((c / num_traces) * 100, c, num_traces))
print("# Success rate at the bit level:   %.2f%% (%d / %d)" % (((num_traces * 2048 - b) / (num_traces * 2048)) * 100, num_traces * 2048 - b, num_traces * 2048))

