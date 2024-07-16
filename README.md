# Simple Power Analysis Attacks using Data with a Single Trace and no Training


These files are part of the Blind-folded work, and provided as artifacts for reproducibility.

Requirements: python3, gcc on a linux or macOS operating system




# RSA Traditional


Usage:

* The main script for recovering the secret exponent from a traditional Square-and-Multiply always exponentiation in libgcrypt is attack.py. Running it without argument will display the options.

A typical use of this script is:
| ``$ python attack.py -p pattern\_0.npy pattern\_1.npy <traces/trace>.npy``

where 'traces/trace.npy' is a power trace of a ligcrypt RSA exponentiation provided on the ftp server. The script will be looking for a file named <traces/trace>\_exp.txt containing the exponent value for result checking. These files are located on the server along with the trace files.




# RSA windowed


Usage:

* The main script for recovering the secret exponent from a sliding windowed exponentiation in libgcrypt is attack.py. Running it without argument will display the options.

A typical use of this script is::

    $ python attack.py -sp pattern\_short\_0.npy pattern\_short\_1.npy <traces/trace>.npy

where 'traces/trace.npy' is a power trace of a ligcrypt RSA exponentiation provided on the ftp server. The script will be looking for a file named <traces/trace>\_exp.txt containing the exponent value for result checking. These files are located on the server along with the trace files. The '-sp' (short patterns) option allows to specifiy that two "short" pattern files must be provided: one for value 0 and one for value 1 of the mask. These correspond to the 50-sample patterns mentioned in the article.


For running the attack using two "medium" pattern files (each 227-sample long), the command is:

    $ python attack.py -mp pattern\_medium\_0.npy pattern\_medium\_1.npy <traces/trace>.npy


For running the attack using a single pattern corresponding the entire ROI, the command is:

    $ python attack.py -lp pattern\_large.npy <traces/trace>.npy




# ECDSA


Usage:

* The attack script for recovering the secret nonce from the trace is attack.py. Running it without argument will display the options.

A typical use of this script is:

    $ python attack.py -t traces/traces\_\*.npy -k traces/keys\_\*.npy

where the trace files and key files are those provided on the ftp server. This will run the attack on the 1000 ECDSA traces.


* The attack script for identifying the zeros is zeros.py.

A typical use of this script is:

    $ python zeros.py -t traces/traces\_\*.npy -k traces/keys_\*.npy

where the trace files and key files are those provided on the ftp server. This will run the attack identifying the zeros on the 1000 ECDSA traces.


NB: These attacks can also be performed on some specific trace files, e.g.:

    $ python attack.py -t traces/traces\_000\_049.npy traces/traces\_050\_099.npy -k traces/keys\_000\_049.npy traces/keys\_050\_099.npy


