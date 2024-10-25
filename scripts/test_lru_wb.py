import os

TRACE_FOLDER = '~/research/c/TRACES'

TRACES = [ 
    'bc', 
    'bfs',
    'bwaves_17',
    'cactuBSSN_17',
    'cam4_17',
    'cc',
    'deepsjeng_17',
    'fotonik3d_17',
    'mcf_17',
    'perlbench_17',
    'sssp',
    'tc',
    'xalancbmk_17'
]

for tr in TRACES:
    cmd = f'''cd builds
             ./Baseline/sim {TRACE_FOLDER}/{tr}.mtf.gz ../ds3conf/base.ini > ../out/{tr}_base.out 2>&1 &
             ./LruWb/sim {TRACE_FOLDER}/{tr}.mtf.gz ../ds3conf/base.ini > ../out/{tr}_lru_wb.out 2>&1 &
           '''
    print(cmd)
    os.system(cmd)
