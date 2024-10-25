'''
    author: Suhas Vittal
    date:   8 October 2024
'''
from sys import argv

# x4 device
params = {
    # DRAM_STRUCTURE
    'protocol': 'DDR5',
    'bankgroups': 8,
    'banks_per_group': 4,
    'rows': 131072,
    'columns': 1024,
    'device_width': 4,
    'BL': 16,
    # SYSTEM
    'channel_size': 16384,
    'channels': 2,
    'bus_width': 32,
    'address_mapping': 'rohirababgchlo',
    'mop_size': 4,
    'queue_structure': 'PER_BANK',
    'refresh_policy': 'RANK_LEVEL_STAGGERED',
    'row_buf_policy': 'OPEN_PAGE',
    'cmd_queue_size': 32,
    'trans_queue_size': 128,
    # TIMING
    'tCK': 0.416,
    'AL': 0,
    'CL': 40,
    'CWL': 38,
    'tRCD': 40,
    'tRP': 40,
    'tRAS': 77,
    'tRFC': 984,
    'tRFCsb': 456,
    'tRFCb': 528,
    'tREFI': 9360,
    'tREFIsb': 1170,
    'tREFIb': 4680,
    'tRRD_S': 8,
    'tRRD_L': 12,
    'tWTR_S': 6,
    'tWTR_L': 24,
    'tFAW': 32,
    'tWR': 72,
    'tRTP': 18,
    'tCCD_S': 8,
    'tCCD_L': 12,
    'tCKE': 8,
    'tCKESR': 13,
    'tXS': 984,
    'tXP': 18,
    'tRTRS': 2,
    # POWER
    # RFM
    'rfm_mode': 0,
    'raaimt': 16,
    'raammt': 48,
    'rfm_raa_decrement': 16,
    'ref_raa_decrement': 16,
    'tRFM': 492
}

DRAM_STRUCTURE = [ 'protocol', 'bankgroups', 'banks_per_group', 'rows',
                  'columns', 'device_width', 'BL' ]
SYSTEM = [ 'channel_size', 'channels', 'bus_width', 'address_mapping',
          'mop_size', 'queue_structure', 'refresh_policy', 'row_buf_policy',
          'cmd_queue_size', 'trans_queue_size' ]
TIMING = [ 'tCK', 'AL', 'CL', 'CWL', 'tRCD', 'tRP', 'tRAS', 'tRFC', 'tRFCsb',
          'tRFCb', 'tREFI', 'tREFIsb', 'tREFIb', 'tRRD_S', 'tRRD_L', 'tWTR_S',
          'tWTR_L', 'tFAW', 'tWR', 'tRTP', 'tCCD_S', 'tCCD_L', 'tCKE', 'tCKESR',
          'tXS', 'tXP', 'tRTRS' ]
RFM = [ 'rfm_mode', 'raaimt', 'raammt', 'rfm_raa_decrement',
       'ref_raa_decrement', 'tRFM' ]

output_file = argv[1]
i = 2
while i < len(argv):
    if argv[i][0] == '-':
        opt = argv[i][1:]
        _x = argv[i+1]
        try:
            x = int(_x)
        except:
            x = _x
        params[opt] = x
        i += 2
    else:
        print(f'Note: ignoring argument \'{argv[i]}\'')
        i += 1
with open(output_file, 'w') as wr:
    sections = [
                ('dram_structure', DRAM_STRUCTURE),
                ('system', SYSTEM),
                ('timing', TIMING),
                ('rfm', RFM) 
            ]
    for (sec, contents) in sections:
        wr.write(f'[{sec}]\n')
        for p in contents:
            x = params[p]
            wr.write(f'{p} = {x}\n')
        wr.write('\n')
    # Other sections (fixed)
    wr.write('''[power] 
VDD = 1.2
IDD0 = 57
IPP0 = 3.0
IDD2P = 25
IDD2N = 37
IDD3P = 43
IDD3N = 52
IDD4W = 150
IDD4R = 168
IDD5AB = 250
IDD6x = 30

[other]
epoch_period = 100000000
output_level = 1
output_prefix = DDR5_baseline
''')
