# hello.py
#import numpy as np
#import xlwings as xw
import subprocess
import os

def run():
    path = os.path.dirname(__file__)
    bin_path = os.path.join(path, '..', 'build/bin/e2rin_multi')
    input_file_path = os.path.join(path, 'in.toml')
    output_file_path = os.path.join(path, 'out.csv')
    stats_file_path = os.path.join(path, 'stats.csv')
    process = subprocess.Popen([bin_path, input_file_path, output_file_path, stats_file_path], stdout=subprocess.PIPE)

#run()