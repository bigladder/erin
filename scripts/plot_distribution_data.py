import sys

import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import pandas as pd


def print_usage(args):
    print(f"python {args[0]} datafile.csv\n" +
            "   This will plot a histogram of the given data and show a plot\n")
    print("Arguments:")
    for idx, a in enumerate(args):
        print(f"- arg[{idx}] = {a}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print_usage(sys.argv)
        sys.exit(1)
    path = sys.argv[1]
    data = pd.read_csv(path)
    data['data'].hist(bins=100)
    plt.show()
    print("Done!")
