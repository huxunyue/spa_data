# Copyright (C) 2024, Sorbonne Universite, LIP6
# This file is part of the Blind-Folded work, under the GPL v3.0 license
# See https://www.gnu.org/licenses/gpl-3.0.en.html for license information
# SPDX-License-Identifier: GPL-3.0-only
# Author(s): Quentin L. Meunier


import sys
import os
import numpy as np




def usage(argv):
    print('Usage: %s -t <trace_file_0>.npy ... <trace_file_n>.npy -k <key_file_0>.npy ... <key_file_n>.npy' % argv[0])
    print('Retrieves the zeros in the scalar used in the ECDSA multiplication')
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




def load_poi(traces):
    pois = []
    poi = 68571
    pois.append(poi)
    for j in range(1, 64):
        if j % 2 == 1:
            poi += 92184
        elif j % 2 == 0:
            poi += 92190
        pois.append(poi)

    traces_poi = [[[0 for x in range(37)] for y in range(64)] for z in range(len(traces))]
    for trace_num in range(len(traces)):
        for it in range(64):
            traces_poi[trace_num][it][0:37] = traces[trace_num][pois[it]:pois[it] + 37]
    
    return traces_poi



num_traces = 0

total_fake_zero = 0
total_fake_non_zero = 0
nb_true_zeros = 0


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
   
    traces_poi = load_poi(trace_set)

    for trace_num in range(len(trace_set)):

        pm = []
        for digit in range(64):
            pm.append(traces_poi[trace_num][digit][2])
        pm.sort()
    
        max_diff = 0
        idx_max = -1
        for i in range(len(pm) - 1):
            diff = pm[i + 1] - pm[i]
            if diff > max_diff:
                max_diff = diff
                idx_max = i
                
        threshold = (pm[idx_max + 1] + pm[idx_max]) / 2
        #print('# Threshold : %f - max_diff : %f' % (threshold, max_diff))
    
        no_zeros = max_diff < 0.01
    
        key = key_set[trace_num]
        key_str = ''.join(map(lambda x: '%08x' % x, key)).upper()
    
        fake_zero = 0 # Found zero, key digit is not zero
        fake_non_zero = 0 # Found value different from zero, key digit is zero
        for digit in range(64):
            
            true_zero = key_str[digit] == '0'
            if not no_zeros and traces_poi[trace_num][digit][2] < threshold and not true_zero:
                print("# fake zero:")
                print('Digit %d' % digit)
                print(traces_poi[trace_num][digit][2])
                print(threshold)
                fake_zero += 1
            elif (no_zeros or traces_poi[trace_num][digit][2] >= threshold) and true_zero:
                print("# fake non zero:")
                print('Digit %d' % digit)
                print(traces_poi[trace_num][digit][2])
                print(threshold)
                fake_non_zero += 1
            nb_true_zeros += (true_zero and 1 or 0)
    
        if fake_zero != 0 or fake_non_zero != 0:
            print('# Trace %d: %d fake 0 found - %d fake non-zero found' % (trace_num, fake_zero, fake_non_zero))

        total_fake_zero += fake_zero
        total_fake_non_zero += fake_non_zero

print('# Summary for %d traces:' % num_traces)
print('# On %d digits equal to zero, %d were correctly identified as zero' % (nb_true_zeros, nb_true_zeros - total_fake_non_zero))
print('# On %d digits different from zero, %d were correctly identified different from zero' % (num_traces * 64 - nb_true_zeros, num_traces * 64 - nb_true_zeros - total_fake_zero))










