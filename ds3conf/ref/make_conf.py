import os

tCK = 0.416

def cyc(x):
    return int( round(x/tCK) )

for tREFW in ['NONE', 8, 16, 32, 64, 128]:
    if tREFW == 'NONE':
        # No refresh
        ref_policy = 'NO_REFRESH'
        tREFI = 18720
    else:
        tREFI_ns = (tREFW*1e6) / (8.0*1024)
        tREFI = cyc(tREFI_ns)
        ref_policy = 'RANK_LEVEL_STAGGERED'
    # Write two configs: one with tWTR = 0 and one without.
    os.system(f'python make_ini.py ref/DDR5_32GB_x4_r{tREFW}.ini -tREFI {tREFI} -refresh-policy {ref_policy}')
