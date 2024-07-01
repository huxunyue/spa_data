import sys
import os
import subprocess
import numpy as np
from Crypto.PublicKey import RSA
from Crypto.Random import random
import hashlib
import string
from Levenshtein import distance

# trace_path = '/Volumes/LaCie/traces_rsa_win/'
trace_path = '/Volumes/LaCie/'
traces_dir = '/Users/huxunyue/idromel_cw/idromel_traces/apps/stm32f3/rsa/traces'
# num_samples = 490000000
num_samples = 535000000
num_traces = 1
n_boucle = 30

# class Crypto.PublicKey.RSA.RsaKey(**kwargs)

# Variables:
# n (integer) – RSA modulus
# e (integer) – RSA public exponent
# d (integer) – RSA private exponent
# p (integer) – First factor of the RSA modulus
# q (integer) – Second factor of the RSA modulus
# invp (integer) – Chinese remainder component ()
# invq (integer) – Chinese remainder component ()
# u (integer)


def change_forme(d):
    s = ''
    for i in range(len(d)//2):
        s += "0x" + d[2*i:2*i+2] + ", "
    return s[:-2]


def write2file(filename, content, mode):
    file = open(filename, mode)
    file.write(content)
    file.close()



c = 0
i = 2
b = 0
while n_boucle > i:

    hash_obj = hashlib.sha224()

    rsa = RSA.generate(2048)
    msg = ''.join(random.sample(string.ascii_letters + string.digits, 28))
    hash_obj.update(msg.encode())

    n = format(rsa.n, '0512x')
    d = format(rsa.d, '0512x')
    base = hash_obj.hexdigest()

    print("base: ",base)
    print("n:    ", n)
    print("d:    ", d)
    # print("len_d: ", len(d))

    from datetime import datetime
    myobj = datetime.now()

    d_file = 'd_values.txt'
    write2file(d_file, 'd [%s:%s:%s] = %s\n' % (myobj.hour, myobj.minute, myobj.second, d), "a")


    content = "%(expo_str)s%(modu_str)s%(base_str)s" % dict(expo_str = d, modu_str = n, base_str = base)
    # print(content)
    file = open("../target_prog/rsa/key.txt", "w")
    file.write(content)


    content = "uint8_t buf_exp[] = {%(expo_str)s};\nuint8_t buf_mod[] = {%(modu_str)s}; \nuint8_t buf_base[] = {%(base_str)s};\n" % dict(expo_str = change_forme(d), modu_str = change_forme(n), base_str = change_forme(base))
    #print(content)

    write2file("../target_prog/rsa/rsa.h", content, "w")


    cmd = ['make', '-C','../target_prog/rsa', 'LIP6DROMEL_HOME=../..', 'TARGET=stm32f3', 'GROUPS=.', 'clean', 'binaries']
    print('# Running command: %s' % ' '.join(map(lambda x: '%s' % x, cmd)))
    ret = subprocess.call(cmd)
    if ret != 0:
        print('*** Error running command, exiting')
        sys.exit(0)


    cmd = ['python', './server.py','-v', '--config', 'server_rsa.cfg', '--traces-dtype', 'float32', '--use-streaming-mode', '--num-samples', '%d' % num_samples, '-b', '%s' % trace_path, '-f', '../build/rsa/stm32f3/rsa.hex', '-c', '%d' % num_traces]
    print('# Running command: %s' % ' '.join(map(lambda x: '%s' % x, cmd)))
    ret = subprocess.call(cmd)
    if ret != 0:
        print('*** Error running command, exiting')
        sys.exit(0)


    traces = np.load(trace_path + "traces.npy")
    print(traces)
    print(traces.shape)

    if traces.size == 0:
        continue

## sauvegarder les traces
    cmd = ['cp', trace_path + "traces.npy", trace_path + "traces%d_30.npy" % i]
    print('# Running command: %s' % ' '.join(map(lambda x: '%s' % x, cmd)))
    ret = subprocess.call(cmd)
    if ret != 0:
        print('*** Error running command, exiting')
        sys.exit(0)

    i += 1


#     if not os.path.exists(traces_dir):
#         os.makedirs(traces_dir)

#     trace_array = np.asarray(traces)
#     # Write trace to file
#     trace_array.astype('float32').tofile(os.path.join(traces_dir, 'traces.raw'))

#     # exit(0)
#     cmd = ["gcc", "-O3", "attack_rsa_7.c", "-o", "rsa"]
#     # cmd = ["gcc", "-O3", "attack_rsa_simple.c", "-o", "rsa"]
#     print('# Running command: %s' % ' '.join(map(lambda x: '%s' % x, cmd)))
#     ret = subprocess.run(cmd, cwd=traces_dir)
#     ret = subprocess.run("./rsa", cwd=traces_dir ,capture_output=True, text=True)
#     print(ret.stdout)

#     key_found = (ret.stdout.split("key found:       0b"))[1].zfill(2048)
#     # print(key_found)
#     key = format(rsa.d, '02048b')
#     # print(key)

#     if(key == key_found):
#         print("Found all bits, correct key!\n")
#         c += 1
#     else:
#         print("%d bits are incorrect or missing :( incorrect key! \n" % distance(key,key_found))
#         b += distance(key,key_found)
#     i += 1


# print("Le taux de reussi niveau trace: %.2f" % ((c/(n_boucle))*100),"% \n")
# print("Le taux de reussi niveau bit: %.2f" % (((2048*n_boucle - b)/(2048*n_boucle))*100),"% \n")
# # print("Le taux de reussi niveau bits: %.2f" % ((((num_traces*2048)-b)/(num_traces*2048))*100),"%")