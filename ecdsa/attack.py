# Copyright (C) 2024, Sorbonne Universite, LIP6
# This file is part of the Blind-Folded work, under the GPL v3.0 license
# See https://www.gnu.org/licenses/gpl-3.0.en.html for license information
# SPDX-License-Identifier: GPL-3.0-only
# Author(s): Xunyue HU


import sys
import os
import numpy as np



def pourcent_reussi(s, in_k):
    c = 0
    b = 0
    for i in range(len(in_k)):
        if in_k[i] == s[i]:
            c += 1
        else:
            print('# Difference for trace %d' % i)
            for j in range(len(in_k[i])):
                if in_k[i][j] != s[i][j]:
                    print('#    Digit %d: key is %s, found %s' % (j, in_k[i][j], s[i][j]))
                    b += 1
    return c, b


def change_endianness(word):
    assert(len(word) == 8)
    return word[6:8] + word[4:6] + word[2:4] + word[0:2]


def le_to_be16(s):
    assert(len(s) % 8 == 0)
    result = ''
    for i in range(int(len(s) / 8)):
        result += change_endianness(s[i * 8:i * 8 + 8])
    return result     #', '.join([str(int("0x" + i, 0)) for i in st.hex()])  // for printing in decimal


def usage(argv):
    print('Usage: %s -t <trace_file_0>.npy ... <trace_file_n>.npy -k <key_file_0>.npy ... <key_file_n>.npy' % argv[0])
    print('Retrieves the scalar used in the ECDSA multiplication')
    print('Options:')
    print('   -t <trace_file>.npy ... <trace_file>.npy: append trace files to the list of trace files to process')
    print('   -k <key_file>.npy ... <key_file>.npy:     append key files to the list of key files to process')
    print('Options -t and -k can be used multiple times, e.g. \'%s -t trace_file_0.npy -k key_file_0.npy key_file_1.npy -t trace_file_1.npy\'.' % argv[0])
    print('The i-th specified trace file must contain a number of traces equal to the number of keys in the i-th key file.')



trace_files = []
key_files = []

idx = 1
mode = ''
while idx < len(sys.argv):
    if sys.argv[idx] == '-t':
        mode = 't'
    elif sys.argv[idx] == '-k':
        mode = 'k'
    else:
        if mode == '':
            print('*** Error: no option specified. Use either \'-t\' or \'-k\' to specify a trace file and a key file respectively.')
            sys.exit(1)
        elif mode == 't':
            trace_files.append(sys.argv[idx])
        elif mode == 'k':
            key_files.append(sys.argv[idx])
        else:
            assert(False)
        
    idx += 1


if len(trace_files) == 0:
    print('*** Error: no trace file given')
    usage(sys.argv)
    sys.exit(1)

if len(key_files) == 0:
    print('*** Error: no key file given')
    usage(sys.argv)
    sys.exit(1)

if len(trace_files) != len(key_files):
    print('*** Error: %d trace files were given while %d key files were given' % (len(trace_files), len(key_files)))
    usage(sys.argv)
    sys.exit(1)



debut = 65372
pois = [[] for i in range(64)]
for j in range(64):
    if (j != 0) and (j % 2 == 1):
        debut += 92184
    elif (j != 0) and (j % 2 == 0):
        debut += 92190
    for i in range(15):
        pois[j].append(debut + 210 * i)


num_traces = 0

found_keys = []
keys_str = []

for idx in range(len(trace_files)):
    trace_file = trace_files[idx]
    if not os.path.isfile(trace_file):
        print('*** Error: No such file \'%s\'' % trace_file)
        sys.exit(1)
    print('# Loading trace file %s' % trace_file)
    trace_set = np.load(trace_file)

    key_file = key_files[idx]
    if not os.path.isfile(key_file):
        print('*** Error: No such file \'%s\'' % key_file)
        sys.exit(1)
    print('# Loading key file %s' % trace_file)
    key_set = np.load(key_file)

    if len(trace_set) != len(key_set):
        print('*** Error: trace file %s has %d traces but key_file %s has %d keys; these file should have the same number of elements' % (trace_file, len(trace_set), key_file, len(key_set)))

    num_traces += len(trace_set)

    print('# Performing the attack')
    for i in range(len(trace_set)):

        trace = trace_set[i]
        key = key_set[i]

        secret = ''
        for j in range(64):
            cons = []
            for i in range(15):
                cons.append(abs(trace[pois[j][i]]))
            if cons[0] > 0.194:
                secret += '1'
            elif max(cons[1:]) < 0.132:
                secret += '0'
            else:
                secret += str(hex(cons[1:].index(max(cons[1:])) + 2)[2])
        found_key = le_to_be16(secret).upper()
        found_keys.append(found_key)
        key_str = ''.join(map(lambda x: change_endianness('%08x' % x), key)).upper()
        keys_str.append(key_str)

c, b = pourcent_reussi(found_keys, keys_str)


print("# Success rate at the trace level: %.2f%% (%d / %d)" % ((c / num_traces) * 100, c, num_traces))
print("# Success rate at the digit level: %.2f%% (%d / %d)" % ((((num_traces * 64) - b) / (num_traces * 64)) * 100, num_traces * 64 - b, num_traces * 64))



