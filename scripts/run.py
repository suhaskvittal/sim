import os
from sys import argv

builds = argv[1:]

TRACE_FOLDER = '~/research/c/TRACES'

TRACES = [ 
    'bc', 
    'bfs',
    'bwaves_17',
    'cactuBSSN_17',
    'cc',
    'fotonik3d_17',
    'mcf_17',
    'sssp',
    'tc',
]

jobs = 0
for tr in TRACES:
    cmd = f'cd builds\n'
    for b in builds:
        cmd += f'./{b}/sim {TRACE_FOLDER}/{tr}.mtf.gz ../ds3conf/base.ini > ../out/{tr}_{b}.out 2>&1 &\n'
    print(cmd)
    os.system(cmd)
    jobs += len(builds)
print('jobs launched:', jobs)
